#ifndef __JZ_PERSISTENCE_H__
#define __JZ_PERSISTENCE_H__

#ifndef __cplusplus
#  error persistence.hpp header must be compiled as C++
#endif

#include "IMat.hpp"
#include "icvstd.hpp"
#include <memory>  // std::shared_ptr

#define ICV_EXPORTS __attribute__ ((visibility ("default")))
namespace JzStereo {
  // FileStorage

  class ICV_EXPORTS FileNode;
  class ICV_EXPORTS FileNodeIterator;

  /** @brief XML/YAML/JSON file storage class that encapsulates all the information necessary for writing or
      reading data to/from a file.
  */
  class  FileStorage
  {
  public:
    //! file storage mode
    enum Mode
    {
      READ        = 0, //!< value, open the file for reading
      WRITE       = 1, //!< value, open the file for writing
      APPEND      = 2, //!< value, open the file for appending
      MEMORY      = 4, //!< flag, read data from source or write data to the internal buffer (which is
      //!< returned by FileStorage::release)
      FORMAT_MASK = (7<<3), //!< mask for format flags
      FORMAT_AUTO = 0,      //!< flag, auto format
      FORMAT_XML  = (1<<3), //!< flag, XML format
      FORMAT_YAML = (2<<3), //!< flag, YAML format
      FORMAT_JSON = (3<<3), //!< flag, JSON format
      BASE64      = 64,     //!< flag, write rawdata in Base64 by default. (consider using WRITE_BASE64)
      WRITE_BASE64 = BASE64 | WRITE, //!< flag, enable both WRITE and BASE64
    };
    enum
    {
      UNDEFINED      = 0,
      VALUE_EXPECTED = 1,
      NAME_EXPECTED  = 2,
      INSIDE_MAP     = 4
    };

    /** @brief The constructors.

	The full constructor opens the file. Alternatively you can use the default constructor and then
	call FileStorage::open.
    */
    FileStorage();

    /** @overload
	@copydoc open()
    */
    FileStorage(const String& filename, int flags, const String& encoding=String());
    //! the destructor. calls release()
    virtual ~FileStorage();

    int state;
    std::string elname;

    class Impl;
    //    Ptr<Impl> p;
    std::shared_ptr<Impl> p;
  };

  /** FileNode
   */
  class FileNode
  {
  public:
    FileNode();
    virtual ~FileNode();
  };

  /** FileIterator is used to control FileNode
   */
  class FileIterator {

  };
}

#endif //__JZ_PERSISTENCE_H__
