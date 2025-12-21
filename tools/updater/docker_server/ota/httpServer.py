# -*- coding: utf-8 -*-
"""
Created on Thu Jul 19 20:41:18 2018

@author: Angus
"""

from __future__ import print_function

from flask import Flask, request, jsonify
import json
app = Flask(__name__)

server_ip = "192.168.2.40"

@app.route('/',methods=['POST'])
def ddd():
    print(request.data)
    #print(request.headers)
    b = json.loads(request.data)
    try:
        print(b["product_name"])
        print(b["sn"])
        print(b["mac"])
        print(b["curr_ver"])
        print(b["client_id"])
        if int(b["curr_ver"]) < 13:
            print("curr_ver =",b["curr_ver"],",update!!")
            json_data = {}
            a = {}
            a.update(ip=server_ip)
            a.update(url="https://"+server_ip+"/version3")
            a.update(rpt_url="")
            a.update(upgrade=str(1))
            a.update(force_upgrade=str(0))
            a.update(curr_ver=b["curr_ver"])
            a.update(new_ver=str(13))
            json_data.update(server=a)
        else:
            print("curr_ver is already new, not update!!")
            json_data = {}
            a = {}
            a.update(ip=server_ip)
            a.update(url="")
            a.update(rpt_url="")
            a.update(upgrade=str(0))
            a.update(force_upgrade=str(0))
            a.update(curr_ver=b["curr_ver"])
            a.update(new_ver=str(13))
            json_data.update(server=a)
        return jsonify(json_data)
    except:
        print("error: json contents error")
        json_data = {}
        a = {}
        a.update(ip=server_ip)
        a.update(url="")
        a.update(rpt_url="")
        a.update(upgrade=str(0))
        a.update(force_upgrade=str(0))
        a.update(curr_ver=b["curr_ver"])
        a.update(new_ver=str(13))
        json_data.update(server=a)
        return jsonify(json_data)

if __name__ == '__main__':
    #app.run(host='0.0.0.0', port=80)
    app.run(host='0.0.0.0', port=8082)
    #app.run('0.0.0.0', debug=True, port=443, ssl_context=('/ssl/server.crt','/ssl/server.key'))

