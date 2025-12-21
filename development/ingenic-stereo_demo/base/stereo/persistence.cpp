#include <internal_persistence.hpp>
#include <memory>  // std::shared_ptr
#include <cstring> // for strlen
#include <vector>

using namespace std;

namespace JzStereo
{
  namespace fs
  {
    int strcasecmp(const char* s1, const char* s2)
    {
      const char* dummy="";
      if(!s1) s1=dummy;
      if(!s2) s2=dummy;

      size_t len1 = strlen(s1);
      size_t len2 = strlen(s2);
      size_t i, len = std::min(len1, len2);
      for( i = 0; i < len; i++ )
	{
	  int d = tolower((int)s1[i]) - tolower((int)s2[i]);
	  if( d != 0 )
            return d;
	}
      return len1 < len2 ? -1 : len1 > len2 ? 1 : 0;
    }

  }
  
  class FileStorage::Impl : public FileStorage_API
  {
  public:
    enum State
      {
	UNDEFINED      = 0,
	VALUE_EXPECTED = 1,
	NAME_EXPECTED  = 2,
	INSIDE_MAP     = 4
      };

    void init()
    {
      // flags = 0;
      // buffer.clear();
      // bufofs = 0;
      // state = UNDEFINED;
      // is_opened = false;
      // dummy_eof = false;
      // write_mode = false;
      // mem_mode = false;
      // space = 0;
      // wrap_margin = 71;
      // fmt = 0;
      // file = 0;
      // gzfile = 0;
      // empty_stream = true;

      // strbufv.clear();
      // strbuf = 0;
      // strbufsize = strbufpos = 0;
      // roots.clear();

      // fs_data.clear();
      // fs_data_ptrs.clear();
      // fs_data_blksz.clear();
      // freeSpaceOfs = 0;

      // str_hash.clear();
      // str_hash_data.clear();
      // str_hash_data.resize(1);
      // str_hash_data[0] = '\0';

      // filename.clear();
      // lineno = 0;
    }

    Impl(FileStorage* _fs)
    {
      // fs_ext = _fs;
      // init();
    }

    virtual ~Impl()
    {
      release();
    }

    void release(String* out=0)
    {
      // if( is_opened )
      // 	{
      // 	  if(out)
      // 	    out->clear();
      // 	  if( write_mode )
      // 	    {
      // 	      while( write_stack.size() > 1 )
      // 		{
      // 		  endWriteStruct();
      // 		}
      // 	      flush();
      // 	      if( fmt == FileStorage::FORMAT_XML )
      // 		puts( "</opencv_storage>\n" );
      // 	      else if ( fmt == FileStorage::FORMAT_JSON )
      // 		puts( "}\n" );
      // 	    }

      // 	  closeFile();
      // 	  if( mem_mode && out )
      // 	    {
      // 	      *out = cv::String(outbuf.begin(), outbuf.end());
      // 	    }
      // 	  init();
      // 	}
    }

    void analyze_file_name( const std::string& file_name, std::vector<std::string>& params )
    {
      // params.clear();
      // static const char not_file_name       = '\n';
      // static const char parameter_begin     = '?';
      // static const char parameter_separator = '&';

      // if( file_name.find(not_file_name, (size_t)0) != std::string::npos )
      // 	return;

      // size_t beg = file_name.find_last_of(parameter_begin);
      // params.push_back(file_name.substr((size_t)0, beg));

      // if( beg != std::string::npos )
      // 	{
      // 	  size_t end = file_name.size();
      // 	  beg++;
      // 	  for( size_t param_beg = beg, param_end = beg;
      // 	       param_end < end;
      // 	       param_beg = param_end + 1 )
      // 	    {
      // 	      param_end = file_name.find_first_of( parameter_separator, param_beg );
      // 	      if( (param_end == std::string::npos || param_end != param_beg) && param_beg + 1 < end )
      // 		{
      // 		  params.push_back( file_name.substr( param_beg, param_end - param_beg ) );
      // 		}
      // 	    }
      // 	}
    }

    bool open( const char* filename_or_buf, int _flags, const char* encoding )
    {
      return false;
    }
  };

  FileStorage::FileStorage()
    : state(0)
  {
    p = make_shared<FileStorage::Impl>(this);
    
  }

  FileStorage::FileStorage(const String& filename, int flags, const String& encoding)
    : state(0)
  {
    p = make_shared<FileStorage::Impl>(this);
    bool ok = p->open(filename.c_str(), flags, encoding.c_str());
    if(ok)
      state = FileStorage::NAME_EXPECTED + FileStorage::INSIDE_MAP;
  }

  FileStorage::~FileStorage()
  {
    
  }

  FileStorage_API::~FileStorage_API()
  {
    
  }
  
}
