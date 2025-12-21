#ifndef SRC_PERSISTENCE_HPP
#define SRC_PERSISTENCE_HPP
#include <persistence.hpp>

namespace JzStereo {
  namespace fs {}
  
  struct FStructData
  {
    FStructData() { indent = flags = 0; }
    FStructData( const std::string& _struct_tag,
		 int _struct_flags, int _struct_indent )
    {
      tag = _struct_tag;
      flags = _struct_flags;
      indent = _struct_indent;
    }

    std::string tag;
    int flags;
    int indent;
  };

  class FileStorage_API
  {
  public:

    virtual ~FileStorage_API();
#if 0    
    virtual FileStorage* getFS() = 0;

    virtual void puts( const char* str ) = 0;
    virtual char* gets() = 0;
    virtual bool eof() = 0;
    virtual void setEof() = 0;
    virtual void closeFile() = 0;
    virtual void rewind() = 0;
    virtual char* resizeWriteBuffer( char* ptr, int len ) = 0;
    virtual char* bufferPtr() const = 0;
    virtual char* bufferStart() const = 0;
    virtual char* bufferEnd() const = 0;
    virtual void setBufferPtr(char* ptr) = 0;
    virtual char* flush() = 0;
    virtual void setNonEmpty() = 0;
    virtual int wrapMargin() const = 0;

    virtual FStructData& getCurrentStruct() = 0;

    virtual void convertToCollection( int type, FileNode& node ) = 0;
    virtual FileNode addNode( FileNode& collection, const std::string& key,
			      int type, const void* value=0, int len=-1 ) = 0;
    virtual void finalizeCollection( FileNode& collection ) = 0;
    virtual double strtod(char* ptr, char** endptr) = 0;

    virtual char* parseBase64(char* ptr, int indent, FileNode& collection) = 0;
    virtual void parseError(const char* funcname, const std::string& msg,
			    const char* filename, int lineno) = 0;
#endif    
  };
}
#endif // SRC_PERSISTENCE_HPP
