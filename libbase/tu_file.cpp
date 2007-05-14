// tu_file.cpp	-- Ignacio Casta�o, Thatcher Ulrich <tu@tulrich.com> 2003

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// A file class that can be customized with callbacks.


#include "tu_file.h"
#include "utility.h"
#include "container.h"
#include "membuf.h"
#include "log.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

//
// tu_file functions using FILE
//

using namespace gnash;

namespace gnash {

static int std_read_func(void* dst, int bytes, void* appdata);
static int std_write_func(const void* src, int bytes, void* appdata);
static int std_seek_func(int pos, void *appdata);
static int std_seek_to_end_func(void *appdata);
static int std_tell_func(void *appdata);
static bool std_get_eof_func(void *appdata);
static int std_get_err_func(void *appdata);
static long std_get_stream_size_func(void *appdata);

static int
std_read_func(void* dst, int bytes, void* appdata) 
// Return the number of bytes actually read.  EOF or an error would
// cause that to not be equal to "bytes".
{
//    GNASH_REPORT_FUNCTION;
    
    assert(appdata);
    assert(dst);
    return fread( dst, 1, bytes, (FILE *)appdata );
}

static int
std_write_func(const void* src, int bytes, void* appdata)
// Return the number of bytes actually written.
{
    assert(appdata);
    assert(src);
    return fwrite( src, 1, bytes, (FILE *)appdata );
}

static int
std_seek_func(int pos, void *appdata)
{
    assert(appdata);

    // TODO: optimize this by caching total stream size ?
    if (pos > std_get_stream_size_func(appdata))
    {
	    return TU_FILE_SEEK_ERROR;
    }

    clearerr((FILE*) appdata);	// make sure EOF flag is cleared.
    int	result = fseek((FILE*)appdata, pos, SEEK_SET);
    if (result == EOF) {
	// @@ TODO should set m_error to something relevant based on errno.
	return TU_FILE_SEEK_ERROR;
    }
    return 0;
}

static int
std_seek_to_end_func(void *appdata)
// Return 0 on success, TU_FILE_SEEK_ERROR on failure.
{
    assert(appdata);
    int	result = fseek((FILE*)appdata, 0, SEEK_END);
    if (result == EOF) {
	// @@ TODO should set m_error to something relevant based on errno.
	return TU_FILE_SEEK_ERROR;
    }
    return 0;
}

static int
std_tell_func(void *appdata)
// Return the file position, or -1 on failure.
{
    assert(appdata);
    FILE* f = static_cast<FILE*>(appdata);

    //if ( feof(f) )
    //assert ( ! feof(f) ); // I guess it's legal to call tell() while at eof.

    int ret = ftell(f);
    assert(ret <= std_get_stream_size_func(appdata));
    return ret;
}

static bool
std_get_eof_func(void *appdata)
// Return true if we're at EOF.
{
    assert(appdata);
    if (feof((FILE*) appdata)) {
	return true;
    } else {
	return false;
    }
}

static int
std_get_err_func(void *appdata)
// Return true if we're at EOF.
{
    if ( ! appdata ) return TU_FILE_OPEN_ERROR;
    return (ferror((FILE*) appdata));
}

static long
std_get_stream_size_func(void *appdata)
// Return -1 on failure, or the size
{
    assert(appdata);

    FILE* f = static_cast<FILE*>(appdata);

    struct stat statbuf;
    if ( -1 == fstat(fileno(f), &statbuf) )
    {
	    log_error("Could not fstat file");
	    return 0;
    }
    return statbuf.st_size;
}


static int
std_close_func(void *appdata)
// Return 0 on success, or TU_FILE_CLOSE_ERROR on failure.
{
    assert(appdata);
    int	result = fclose((FILE*)appdata);
    if (result == EOF) {
	// @@ TODO should set m_error to something relevant based on errno.
	return TU_FILE_CLOSE_ERROR;
    }
    return 0;
}


//
// tu_file functions using a readable/writable memory buffer
//

class filebuf
{
public:

    membuf 	m_;
    int		m_position;
    
    filebuf()
	{
		m_position = 0; 
		m_read_only = false;
	}
    
    filebuf(int size, void* data)
	:
	m_(membuf::READ_ONLY, data, size),
	m_position(0),
	m_read_only(true)
	{
	}
    
    ~filebuf()
	{
	}
    
    bool	resize(int new_size)
	// Return false if we couldn't resize.
	{
	    if (m_read_only) {
		return false;
	    }
	    
	    m_.resize(new_size);
	    
	    // Hm, does this make sense?  We're truncating the file, so clamping the cursor.
	    // Alternative would be to disallow resize, but that doesn't seem good either.
	    if (m_position > m_.size())	{
		m_position = m_.size();
	    }
	    
	    return true;
	}
    
    bool	is_valid()
	{
	    return
		m_position >= 0
		&& m_position <= m_.size();
	}
    
    unsigned char*	get_cursor()
	{
	    return ((unsigned char*) m_.data()) + m_position;
	}

private:
    bool	m_read_only;	
};


static int
mem_read_func(void* dst, int bytes, void* appdata) 
// Return the number of bytes actually read.  EOF or an error would
// cause that to not be equal to "bytes".
{
    assert(appdata);
    assert(dst);
    
    filebuf* buf = (filebuf*) appdata;
    assert(buf->is_valid());
    
    int	bytes_to_read = imin(bytes, buf->m_.size() - buf->m_position);
    if (bytes_to_read) {
	memcpy(dst, buf->get_cursor(), bytes_to_read);
    }
    buf->m_position += bytes_to_read;
    
    return bytes_to_read;
}


static int mem_write_func(const void* src, int bytes, void* appdata)
// Return the number of bytes actually written.
{
    assert(appdata);
    assert(src);
    
    filebuf* buf = (filebuf*) appdata;
    assert(buf->is_valid());
    
    // Expand buffer if necessary.
    int	bytes_to_expand = imax(0, buf->m_position + bytes - buf->m_.size());
    if (bytes_to_expand) {
	if (buf->resize(buf->m_.size() + bytes_to_expand) == false) {
	    // Couldn't expand!
	    return 0;
	}
    }
    
    memcpy(buf->get_cursor(), src, bytes);
    buf->m_position += bytes;
    
    return bytes;
}

static int
mem_seek_func(int pos, void *appdata)
// Return 0 on success, or TU_FILE_SEEK_ERROR on failure.
{
    assert(appdata);
    assert(pos >= 0);
    
    filebuf* buf = (filebuf*) appdata;
    assert(buf->is_valid());
    
    if (pos < 0) {
	buf->m_position = 0;
	return TU_FILE_SEEK_ERROR;
    }
    
    if (pos > buf->m_.size()) {
	buf->m_position = buf->m_.size();
	return TU_FILE_SEEK_ERROR;
    }
    
    buf->m_position = pos;
    return 0;
}

static int
mem_seek_to_end_func(void* appdata)
// Return 0 on success, TU_FILE_SEEK_ERROR on failure.
{
    assert(appdata);
    
    filebuf* buf = (filebuf*) appdata;
    assert(buf->is_valid());
    
    buf->m_position = buf->m_.size();
    return 0;
}

static int
mem_tell_func(void* appdata)
// Return the file position, or -1 on failure.
{
    assert(appdata);
    
    filebuf* buf = (filebuf*) appdata;
    assert(buf->is_valid());
    
    return buf->m_position;
}

static bool
mem_get_eof_func(void* appdata)
// Return true if we're positioned at the end of the buffer.
{
    assert(appdata);
    
    filebuf* buf = (filebuf*) appdata;
    assert(buf->is_valid());
    
    return buf->m_position >= buf->m_.size();
}

static int
mem_get_err_func(void* appdata)
// Return true if we're positioned at the end of the buffer.
{
    filebuf* buf = (filebuf*) appdata;
    return buf->is_valid();
}

static long
mem_get_stream_size(void* appdata)
// Return the file size, or -1 on failure.
{
    assert(appdata);
    
    filebuf* buf = (filebuf*) appdata;
    assert(buf->is_valid());
    
    return buf->m_.size();
}

static int mem_close_func(void* appdata)
// Return 0 on success, or TU_FILE_CLOSE_ERROR on failure.
{
    assert(appdata);
    
    filebuf* buf = (filebuf*) appdata;
    assert(buf->is_valid());
    
    delete buf;
    
    return 0;
}
}

//
// generic functionality
//

// Create a file using the custom callbacks.
tu_file::tu_file(void * appdata, read_func rf, write_func wf,
		 seek_func sf, seek_to_end_func ef, tell_func tf,
		 get_eof_func gef, get_err_func ger, get_stream_size_func gss, close_func cf)
{
    m_data = appdata;
    m_read = rf;
    m_write = wf;
    m_seek = sf;
    m_seek_to_end = ef;
    m_tell = tf;
    m_get_eof = gef;
    m_get_err = ger;
    m_get_stream_size = gss;
    m_close = cf;
}

// Create a file from a standard file pointer.
tu_file::tu_file(FILE* fp, bool autoclose=false)
{
    //GNASH_REPORT_FUNCTION;

    m_data = (void *)fp;
    m_read = std_read_func;
    m_write = std_write_func;
    m_seek = std_seek_func;
    m_seek_to_end = std_seek_to_end_func;
    m_tell = std_tell_func;
    m_get_eof = std_get_eof_func;
    m_get_err = std_get_err_func;
    m_get_stream_size = std_get_stream_size_func;
    m_close = autoclose ? std_close_func : NULL;
}

// Create a file from the given name and the given mode.
tu_file::tu_file(const char * name, const char * mode)
{
	GNASH_REPORT_RETURN;

	m_data = fopen(name, mode);
    
	m_read = std_read_func;
	m_write = std_write_func;
	m_seek = std_seek_func;
	m_seek_to_end = std_seek_to_end_func;
	m_tell = std_tell_func;
	m_get_eof = std_get_eof_func;
	m_get_err = std_get_err_func;
	m_get_stream_size = std_get_stream_size_func;
	m_close = std_close_func;
}

tu_file::tu_file(memory_buffer_enum /* m */)
// Create a read/write memory buffer.
{
    m_data = new membuf;
    
    m_read = mem_read_func;
    m_write = mem_write_func;
    m_seek = mem_seek_func;
    m_seek_to_end = mem_seek_to_end_func;
    m_tell = mem_tell_func;
    m_get_eof = mem_get_eof_func;
    m_get_err = mem_get_err_func;
    m_get_stream_size = mem_get_stream_size;
    m_close = mem_close_func;
}


tu_file::tu_file(memory_buffer_enum /* m */, int size, void* data)
// Create a read-only memory buffer, using the given data.
{
    m_data = new membuf(data, size);
    
    m_read = mem_read_func;
    m_write = mem_write_func;
    m_seek = mem_seek_func;
    m_seek_to_end = mem_seek_to_end_func;
    m_tell = mem_tell_func;
    m_get_eof = mem_get_eof_func;
    m_get_err = mem_get_err_func;
    m_get_stream_size = mem_get_stream_size;
    m_close = mem_close_func;
}

tu_file::~tu_file()
// Close this file when destroyed.
{
    close();
}

void
tu_file::close() 
// Close this file.
{ 
    if (m_close && m_data) {
	m_close(m_data);
    }
    m_data = NULL; 
    m_read = NULL; 
    m_write = NULL; 
    m_seek = NULL; 
    m_tell = NULL; 
    m_close = NULL; 
}


void
tu_file::copy_from(tu_file* src)
// Copy remaining contents of *src into *this.
{
    // @@ bah, should buffer this!
    while (src->get_eof() == false) {
	uint8_t	b = src->read8();
	if (src->get_error()) {
	    break;
	}
	
	write8(b);
    }
}


void
tu_file::copy_to(membuf* dst)
// Copy remaining contents of *this into *dst.
{
    static const int BUFSIZE = 4096;
    
    while (get_eof() == false) {
	// Make room at the end of dst.
	dst->resize(dst->size() + BUFSIZE);
	int bytes_read = read_bytes(((char*) dst->data()) + dst->size() - BUFSIZE, BUFSIZE);
	if (bytes_read < BUFSIZE) {
	    // Didn't use everything we allocated; trim the unused bytes.
	    dst->resize(dst->size() - (BUFSIZE - bytes_read));
	}
	
	if (get_error()) {
	    break;
	}
    }
}


int
tu_file::copy_bytes(tu_file* src, int byte_count)
// Copy a fixed number of bytes from *src into *this.  Return the
// number of bytes copied.
{
    static const int	BUFSIZE = 4096;
    char	buffer[BUFSIZE];
    
    int	bytes_left = byte_count;
    while (bytes_left) {
	int	to_copy = imin(bytes_left, BUFSIZE);
	
	int	read_count = src->read_bytes(buffer, to_copy);
	int	write_count = write_bytes(buffer, read_count);
	
	assert(write_count <= read_count);
	assert(read_count <= to_copy);
	assert(to_copy <= bytes_left);
	
	bytes_left -= write_count;
	if (write_count < to_copy) {
	    // Some kind of error; abort.
	    return byte_count - bytes_left;
	}
    }
    
    assert(bytes_left == 0);
    
    return byte_count;
}

void
tu_file::write_string(const char* src)
{
    for (;;) {
	write8(*src);
	if (*src == 0) {
	    break;
	}
	src++;
    }
}

int
tu_file::read_string(char* dst, int max_length) 
{
    int i=0;
    while (i<max_length) {
	dst[i] = read8();
	if (dst[i]=='\0') {
	    return i;
	}
	i++;
    }
    
    dst[max_length - 1] = '\0';	// force termination.
    
    return -1;
}



#include <cstdarg>
#include <cstring>

#ifdef _WIN32
#define vsnprintf	_vsnprintf
#endif // _WIN32

int	tu_file::printf(const char* fmt, ...)
// Use printf-like semantics to send output to this stream.
{
    // Workspace for vsnprintf formatting.
    static const int	BUFFER_SIZE = 1000;
    char	buffer[BUFFER_SIZE];
    
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buffer, BUFFER_SIZE, fmt, ap);
    va_end(ap);
    
    return write_bytes(buffer, strlen(buffer));
}


// Local Variables:
// mode: C++
// indent-tabs-mode: t
// End:
