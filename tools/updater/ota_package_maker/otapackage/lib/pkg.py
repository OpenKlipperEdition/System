#!bin/env python
# --*-- coding:utf8 --*--

import os
import sys
import stat
import zipfile
import hashlib
import xml.etree.cElementTree as et
from otapackage import config

class Param:
    def __init__(self,otadiffdir,otadiffscript,imgname,imgtype,totallimitsize,limitsize,pkgindex,newexternpath):
        self.imgname=imgname
        self.imgtype=imgtype
        self.otadiffdir=otadiffdir#"/ota-diff-package"
        self.otadiffscript=otadiffscript#"/otadiff.sh"
        self.totallimitsize=totallimitsize#default 300*1024*1024
        self.limitsize=limitsize#default 1048576
        self.pkgindex=pkgindex#1
        self.newexternpath=newexternpath

class Handler:
    def dirhandler(self, src, dst):
        pass

    def filehandler(self, src, dst):
        pass

    def linkhandle(self, src, dst):
        pass

    def writetoscript(self, scriptpath, cmd, p):
        #print("***scriptpath=%s,cmd=%s" %(scriptpath,cmd))
        with open(scriptpath+p.otadiffscript,'a') as script:
            script.write("%s" %(cmd)+'\n')

    def calcSha1(self, filepath):
        with open(filepath,'rb') as f:
            sha1obj = hashlib.sha1()
            sha1obj.update(f.read())
            hash = sha1obj.hexdigest()
        print ("%s:hash:%s" %(filepath,hash))
        return hash

    def getdirsize(self, rootdir):
        size = 4096
        for filename in os.listdir(rootdir):
            pathname = os.path.join(rootdir, filename)
            if os.path.islink(pathname):
                size += (len(os.readlink(pathname))  + 4095) / 4096 * 4096
            elif os.path.isfile(pathname):
                size += (os.path.getsize(pathname) + 4095) / 4096 * 4096
            else:
                size += self.getdirsize(pathname)
        return size;

    def getexternpath(self, size, p):
        if p.pkgindex == 0:
            p.pkgindex = p.pkgindex + 1
        else:
            dirsize=self.getdirsize(p.otadiffdir+"/update"+str("{0:03d}".format(p.pkgindex)))
            if dirsize+size > p.totallimitsize:
                    print("\npackage limit size is %d Bytes" %(p.totallimitsize))
                    print("the diff package is too large, stop!!!please try full package update ...\n")
                    os.system("rm -rf "+p.otadiffdir)
                    os._exit(0)
            if p.limitsize != p.totallimitsize and dirsize+size > p.limitsize:
                p.pkgindex = p.pkgindex + 1

        externpath = p.otadiffdir+"/update"+str("{0:03d}".format(p.pkgindex))
        if not os.path.exists(externpath):
            os.makedirs(externpath)
            self.writetoscript(externpath, "set -e", p)

        return externpath

    def getlocalpath(self, src, dst, p):
        localpath=''
        resultpath=''
        cmpcount = 0
        srclist = src.split("/")
        dstlist = dst.split("/")
        srcdircount = len(srclist)
        dstdircount = len(dstlist)
        while cmpcount < srcdircount and cmpcount < dstdircount:
            cmpcount = cmpcount + 1
            if not cmp(srclist[srcdircount - cmpcount], dstlist[dstdircount - cmpcount]):
                localpath = "/" + srclist[srcdircount - cmpcount]+localpath
            else:
                break
        if(p.imgtype == "directory"):
            resultpath = localpath[len(p.imgname)+1:len(localpath)]
        else:
            resultpath = localpath
        #print("localpath:%s" %(resultpath))
        #print("resultpath:%s" %(resultpath))
        return resultpath

    #探测path是否存在，如果不存在，创建
    def detectdir(self, path):
        if not os.path.isdir(os.path.dirname(path)):
            os.makedirs(os.path.dirname(path))

    def splitfile(self,src,dst,p):
        chunknum=0
        currentsize=p.limitsize-self.getdirsize(p.otadiffdir+"/update"+str("{0:03d}".format(p.pkgindex)))
        if currentsize > 512:
            chunksize=currentsize-512 #预留512字节给cmd
        else:
            chunksize=p.limitsize-512 #预留512字节给cmd
        inputfile=open(src, 'rb') #rb 读二进制文件
        while 1:
            chunk=inputfile.read(chunksize)
            if not chunk: #文件块是空的
                break
            chunknum+=1
            pkgpath=self.getexternpath(chunksize+512, p)
            filename=pkgpath+("%s-%03d" % (src[src.rfind("/"):],chunknum))
            with open(filename,'wb') as fileobj:
                fileobj.write(chunk)
            self.writetoscript(pkgpath,"cat $2"+("%s-%03d" % (src[src.rfind("/"):],chunknum))+" >>"+"$1"+self.getlocalpath(src,dst,p), p)
            chunksize=p.limitsize-512

        cmd = "chmod "+str(oct(stat.S_IMODE(os.stat(src).st_mode)))+" "+"$1"+self.getlocalpath(src,dst,p)
        pkgpath = self.getexternpath(len(cmd)+1, p)
        self.writetoscript(pkgpath, cmd, p)
        inputfile.close()

        cmd="sha1text=`sha1sum $1%s`" %self.getlocalpath(src,dst,p)
        cmd=cmd+"\n"+"if [ x${sha1text%%' '*} != x%s ];then" %self.calcSha1(src)
        cmd=cmd+"\n"+"echo 'splitfile sha1 %s error,hash'" %self.getlocalpath(src,dst,p)
        cmd=cmd+"\n"+"exit 111"
        cmd=cmd+"\n"+"fi"
        pkgpath = self.getexternpath(len(cmd)+1, p)
        self.writetoscript(pkgpath, cmd, p)

    def fileprocess(self, src, dst, p):
        cmd = "cp -a "+"$2"+self.getlocalpath(src,dst,p)+" "+"$1"+os.path.dirname(self.getlocalpath(src,dst,p))
        if (os.path.getsize(src)+len(cmd)+1) < p.limitsize:
            pkgpath = self.getexternpath(os.path.getsize(src)+len(cmd)+1, p)
            self.detectdir(pkgpath+self.getlocalpath(src,dst,p))
            os.system("cp -a "+src+" "+pkgpath+os.path.dirname(self.getlocalpath(src,dst,p)))
            self.writetoscript(pkgpath, cmd, p)
        else:
            self.splitfile(src, dst,p)

class SrcDir(Handler):
    #只处理a存在且不是link文件，b是link文件或者不存在情况(无论a是目录还是文件)
    def dirhandler(self, src, dst, p):
        cmd = "rm -rf "+"$1"+self.getlocalpath(src,dst,p)
        path = self.getexternpath(len(cmd)+1, p)
        self.writetoscript(path, cmd, p)

    def diff(self, src, dst, p):
        cmd = ''
        returnStatus=os.system("diff "+src+" "+dst+" > /dev/null")
        if returnStatus == 256 or returnStatus == 512: #src和dst有区别
            if returnStatus == 256: #文件是text
                cmd="sha1text=`sha1sum $1%s`" %self.getlocalpath(src,dst,p)
                cmd=cmd+"\n"+"if [ x${sha1text%%' '*} = x%s ];then" %self.calcSha1(src)
                cmd=cmd+"\n"+"patch "+"$1"+self.getlocalpath(src,dst,p)+" < "+"$2"+self.getlocalpath(src,dst,p)+".patch"
                cmd=cmd+"\n"+"else"
                cmd=cmd+"\n"+"echo 'sha1 %s error,hash'" %self.getlocalpath(src,dst,p)
                cmd=cmd+"\n"+"exit 111"
                cmd=cmd+"\n"+"fi"
                os.system("diff -au "+src+" "+dst+" > "+p.otadiffdir+src[src.rfind("/"):]+".patch")
            elif returnStatus == 512: #文件是二进制
                cmd="/usr/sbin/bspatch "+"$1"+self.getlocalpath(src,dst,p)+" "+"$1"+self.getlocalpath(src,dst,p)+" "+self.calcSha1(dst)+" "+str(os.path.getsize(dst))+" "+self.calcSha1(src)+":"+"$2"+self.getlocalpath(src,dst,p)+".patch"
                os.system("./bsdiff "+src+" "+dst+" "+p.otadiffdir+src[src.rfind("/"):]+".patch")

            size=os.path.getsize(p.otadiffdir+src[src.rfind("/"):]+".patch")+len(cmd)+1
            if size < p.limitsize:
                pkgpath = self.getexternpath(size, p)
                self.detectdir(pkgpath+self.getlocalpath(src,dst,p))
                os.system("mv "+p.otadiffdir+src[src.rfind("/"):]+".patch"+" "+ pkgpath + self.getlocalpath(src,dst,p)+".patch")
                self.writetoscript(pkgpath, cmd, p)

                if not stat.S_IMODE(os.stat(src).st_mode) == stat.S_IMODE(os.stat(dst).st_mode):
                    cmd = "chmod "+str(oct(stat.S_IMODE(os.stat(dst).st_mode)))+" "+"$1"+self.getlocalpath(src,dst,p)
                    pkgpath = self.getexternpath(len(cmd)+1, p)
                    self.writetoscript(pkgpath, cmd, p)
            else: #patch大于limitsize,直接拷贝源文件
                os.system("rm -f "+p.otadiffdir+src[src.rfind("/"):]+".patch")
                cmd="rm -f "+"$1"+self.getlocalpath(src,dst,p)
                pkgpath = self.getexternpath(len(cmd)+1, p)
                self.writetoscript(pkgpath, cmd, p)
                self.fileprocess(dst, src, p)

    #处理a是普通文件(不是link文件也不是文件夹)，b存在且不是link文件(但不知道是文件夹还是文件的情况)
    def filehandler(self, src, dst, p):
        if not os.path.isdir(dst):
            self.diff(src, dst, p)
        else:
            cmd = "rm -f "+"$1"+self.getlocalpath(src,dst,p)
            pkgpath = self.getexternpath(len(cmd)+1, p)
            self.writetoscript(pkgpath, cmd, p)

    #a是link文件，b不知道任何情况(是否存在，是什么文件)
    def linkhandler(self, src, dst, p):
        if os.path.islink(dst):
            if cmp(os.readlink(src), os.readlink(dst)):
                cmd = "cp -a "+"$2"+self.getlocalpath(src,dst,p)+" "+"$1"+os.path.dirname(self.getlocalpath(src,dst,p))
                pkgpath = self.getexternpath(len(os.readlink(dst))+len(cmd)+1, p)
                self.detectdir(pkgpath+self.getlocalpath(src,dst,p))
                os.system("cp -a "+dst+" "+pkgpath+os.path.dirname(self.getlocalpath(src,dst,p)))
                self.writetoscript(pkgpath, cmd, p)
        else:
            cmd = "rm -f "+"$1"+self.getlocalpath(src,dst,p)
            pkgpath = self.getexternpath(len(cmd)+1, p)
            self.writetoscript(pkgpath, cmd, p)

class DstDir(Handler):
    #获取dst相对于src的路径
    def getdirlocalpath(self,src,dst, p):
        localpath=''
        cmpcount = 0
        srclist = src.split("/")
        newlist = dst.split("/")
        while cmpcount < (len(newlist)-len(srclist)):
            cmpcount = cmpcount+1
            localpath="/"+newlist[len(newlist)-cmpcount]+localpath
        #print "localpath:"+localpath
        return localpath

    def dirprocess(self, src, dst, cmd, p):
        for filename in os.listdir(src):
            pathname = os.path.join(src, filename)
            if os.path.islink(pathname) or os.path.isfile(pathname) or (os.path.isdir(pathname) and not os.listdir(pathname)):
                if os.path.islink(pathname):
                    filesize=(len(os.readlink(pathname))  + 4095) / 4096 * 4096
                else:
                    filesize=(os.path.getsize(pathname) + 4095) / 4096 * 4096

                if (filesize+len(cmd)+1) >= p.limitsize:
                    localpath=self.getlocalpath(src,dst,p)+self.getdirlocalpath(src,pathname,p)
                    pkgpath = self.getexternpath(len("mkdir -p "+"$1"+os.path.dirname(localpath))+1, p)
                    self.writetoscript(pkgpath, "mkdir -p "+"$1"+os.path.dirname(localpath), p)

                    self.splitfile(pathname, dst+self.getdirlocalpath(src,pathname,p),p)
                    p.newexternpath=1
                else:
                    recordindex=p.pkgindex
                    pkgpath = self.getexternpath(filesize+len(cmd)+1, p)
                    if p.newexternpath or recordindex<p.pkgindex:
                        self.writetoscript(pkgpath, cmd, p)
                        p.newexternpath=0
                    self.detectdir(pkgpath+self.getlocalpath(src,dst,p)+self.getdirlocalpath(src,pathname,p))
                    os.system("cp -a "+pathname+" "+pkgpath+self.getlocalpath(src,dst,p)+os.path.dirname(self.getdirlocalpath(src,pathname,p)))

            else:
                self.dirprocess(pathname, dst+self.getdirlocalpath(src,pathname,p), cmd, p)

    #只处理b存在且不是link文件，a是link文件或者不存在情况(无论b是目录还是文件)
    def dirhandler(self, src, dst, p):
        cmd="cp -a "+"$2"+self.getlocalpath(src,dst,p)+" "+"$1"+os.path.dirname(self.getlocalpath(src,dst,p))
        if os.path.isdir(src):
            if not os.listdir(src): #空文件夹
                pkgpath = self.getexternpath(4096+len(cmd)+1, p)
                self.detectdir(pkgpath+self.getlocalpath(src,dst,p))
                os.system("cp -a "+src+" "+pkgpath+os.path.dirname(self.getlocalpath(src,dst,p)))
                self.writetoscript(pkgpath,cmd,p)
            else:
                p.newexternpath=1
                self.dirprocess(src,dst,cmd,p)
        else:
            self.fileprocess(src, dst, p)

    #处理b是普通文件(不是link文件也不是文件夹)，a存在且不是link文件(但不知道是文件夹还是文件的情况),不用处理
    def filehandler(self, src, dst, p):
        if os.path.isdir(dst):
            cmd = "rm -rf "+"$1"+self.getlocalpath(src,dst,p)
            pkgpath = self.getexternpath(len(cmd)+1, p)
            self.writetoscript(pkgpath,cmd,p)

            self.fileprocess(src, dst, p)

    #b是link文件,a不知道任何情况(是否存在，是什么文件)
    def linkhandler(self, src, dst, p):
        if not os.path.islink(dst):
            #$1是otadiff.sh脚本的第一个参数，是当前文件系统路径，$2是第二个参数，是差分包所在的路径，注意必须是绝对路径
            cmd = "cp -a "+"$2"+self.getlocalpath(src,dst,p)+" "+"$1"+os.path.dirname(self.getlocalpath(src,dst,p))
            pkgpath = self.getexternpath(len(os.readlink(src))+len(cmd)+1, p)
            self.detectdir(pkgpath+self.getlocalpath(src,dst,p))
            os.system("cp -a "+src+" "+pkgpath+os.path.dirname(self.getlocalpath(src,dst,p)))
            self.writetoscript(pkgpath, cmd,p)

def listalldir(src, dst, handler, needparam):
    if os.path.isdir(src):
        for d in os.listdir(src):
            srcdir = src + "/" + d
            dstdir = dst + "/" + d
            print ("srcdir:"+srcdir)
            if not os.path.islink(srcdir):
                if (os.path.islink(dstdir) == False) and os.path.exists(dstdir):
                    if os.path.isdir(srcdir):
                        listalldir(srcdir, dstdir, handler, needparam)
                    else:
                        handler.filehandler(srcdir, dstdir, needparam)
                else:
                    handler.dirhandler(srcdir, dstdir, needparam)
            else:
                handler.linkhandler(srcdir, dstdir, needparam)
    else:
        handler.filehandler(os.path.abspath(src), os.path.abspath(dst), needparam)

def pkggenerate(srcpath, dstpath, totallimitsize, slicesize, startidx, imgname, imgtype, outputpath):
    a = SrcDir()
    b = DstDir()
    p = Param(outputpath,"/otadiff.sh",imgname,imgtype,totallimitsize,slicesize,startidx,1)

    path=p.otadiffdir+"/update"+str("{0:03d}".format(p.pkgindex))
    if not os.path.exists(path):
        os.makedirs(path)

    listalldir(srcpath, dstpath, a, p)
    listalldir(dstpath, srcpath, b, p)

    for root, dirs, files in os.walk(p.otadiffdir):
        for file in files:
            if file == p.otadiffscript[1:]:
                os.chmod(os.path.join(root,file), stat.S_IRWXO|stat.S_IRWXG|stat.S_IRWXU)

    return p.pkgindex

