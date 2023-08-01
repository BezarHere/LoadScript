#pragma once
#include <fstream>
#include <memory>

#define fatel(x) throw std::runtime_error("")
#define error(x) std::cout << 

namespace bite {

	typedef std::unique_ptr<char[]> BufferSmartPtr_t;
	typedef std::shared_ptr<char[]> BufferSharedPtr_t;
	typedef std::vector<BufferSharedPtr_t> BufferVector_t;

	class File
	{
	public:

		/// TODO: Implement utf-8
		enum OpenMode : uint8_t
		{
			WriteBinary,
			ReadBinary,
			WriteAscii,
			ReadAscii,
			//WriteUTF8, 
			//ReadUTF8,
		};

	public:
		File() = delete;
		File(const std::string &path, const OpenMode mode = OpenMode::ReadAscii);

		bool isForReading() const;
		bool isForWriting() const;
		const std::string &getPath() const;
		const size_t &size() const;
		const size_t &space() const;
		const size_t &tell(bool update = true);
		const size_t &tell() const;
		void seek(size_t index);

		OpenMode mode() const;
		bool isAscii() const;
		bool isBinary() const;

		bool good() const;

		// if fail is true, all reading and writing will fail
		// failing happens if the path is invalid or the program can't access the file ( former is more propeple )
		bool fail() const;

		std::ios::iostate state() const;

		// brief writes the buffer to the file
		// - if 'to' is set to a value smaller or equal to the paramter 'from' or remained the default value (0)
		//   then the buffer will be written to the end
		virtual void write(const char *const buffer, size_t from = 0U, size_t to = 0U);

		// if length is set to value larger then 0 then the set length of bytes will be returned
		// otherwise the rest of the file will be returned ( Not recomended )
		// CALLER DELETES POINTER
		virtual BufferSmartPtr_t read(size_t len = 0U);
		virtual void load(char *buffer, size_t len);

		// flushes the buffer to the file system, called automaticly on closing
		void flush();

		// closes and flushes, making the file accessible for writing by other processes
		void close();

		bool valid() const;

		bool canRead() const;
		bool canWrite() const;

		// [syntax sugar] same as valid
		// if the stream is valid this will return true, otherwise false
		operator bool() const;

	private:
		void recalculate();
	private:
		std::fstream m_stream;
		const OpenMode m_mode;
		const std::string m_path;
		size_t m_size;
		size_t m_space;
		size_t m_cursor;
	};

	enum class EndianOrder
	{
		Little,
		Big
	};

	template <typename STREAM>
	class StreamFrame
	{
	public:
		using stream_type = STREAM;
		
		StreamFrame();
		StreamFrame(std::shared_ptr<STREAM> stream, EndianOrder order);

		void setStream(STREAM *stream);
		virtual void setStream(std::shared_ptr<STREAM> stream);
		std::shared_ptr<STREAM> getStream();
		const std::shared_ptr<const STREAM> getStream() const;

		EndianOrder order() const;

		virtual void move(intptr_t offset) = 0;

		virtual size_t cursor() const = 0;
		virtual size_t size() const = 0;

		std::ios::iostate state() const;
		bool valid() const;
	
		// same as valid
		operator bool() const;
		_NODISCARD bool operator!() const;

	protected:
		const EndianOrder m_order;
		std::shared_ptr<STREAM> m_stream;
	};

	class StreamReader : public StreamFrame<std::istream>
	{
	public:
		StreamReader();
		StreamReader(std::shared_ptr<stream_type> stream, EndianOrder order);
		StreamReader(stream_type *stream, EndianOrder order);
		StreamReader(const std::string &path, EndianOrder order);

		void move(intptr_t offset) override;

		size_t cursor() const override;
		size_t size() const override;

		// [risky to use whith objects]
		// creates a new instance of 'T' and returns it after loading it
		// if ORDER is true then the endian ordering will be apllied
		// when reading integrals/reals 'ORDER' should always be true to make sure accurate readins
		template <typename T, bool ORDER = true>
		T read();

		// [risky to use whith objects]
		// reads without advancing
		// if ORDER is true then the endian ordering will be apllied
		// when reading integrals/reals 'ORDER' should always be true to make sure accurate readins
		template <typename T, bool ORDER = true>
		T peek();

		// 'do_endianeness' can be true if you want an accurate integrale/real value loading
		// but 'do_endianeness' mostly bugs with object loading ( wich you shouldn't do )
		void load(char *buffer, size_t length, bool do_endianeness = false);
		BufferSmartPtr_t read(size_t length);
		BufferSmartPtr_t readCstr(size_t length);
		BufferSmartPtr_t peek(size_t length);
	};

	class StreamWriter : public StreamFrame<std::ostream>
	{
	public:
		StreamWriter();
		StreamWriter(std::shared_ptr<stream_type> stream, EndianOrder order);
		StreamWriter(stream_type *stream, EndianOrder order);
		StreamWriter(const std::string &path, EndianOrder order);

		~StreamWriter();

		void move(intptr_t offset) override;

		size_t cursor() const override;
		size_t size() const override;

		void flush();
		void close();

		// if order, the current endian order is applyed to make sure integrale/real numpers are written correctly
		// rememper to calls flush to flush content to file
		template <typename T, bool ORDER = true>
		void write(const T& value);
		
		// rememper to calls flush to flush content to file
		void write(const char *buffer, size_t length);

		// will not advance cursor like write()
		// rememper to calls flush to flush content to file
		void insert(const char *buffer, size_t length);

		// rememper to calls flush to flush content to file
		void writeCstr(const char *buffer, size_t length);

		// will not advance cursor like writeCstr()
		// rememper to calls flush to flush content to file
		void insertCstr(const char *buffer, size_t length);
	};

	namespace cmddye
	{

		enum class CMDColor : uint8_t
		{
			Black,
			Blue,
			Green,
			Aqua,
			Red,
			Purple,
			Yellow,
			White,
			Gray,
			LightBlue,
			LightGreen,
			LightAqua,
			LightRed,
			LightPurple,
			LightYellow,
			BrightWhite
		};

		struct CMDTheme {
			CMDColor fg, bg = CMDColor::Black;
		};

		extern CMDTheme M_GetTheme();
		extern CMDTheme M_GetDefaultTheme();
		extern void M_PutTheme(CMDTheme theme);
		// will return to the same theme the program started with
		extern void M_ClearTheme();

		extern constexpr uint8_t M_UnpackTheme(CMDTheme theme);
		extern constexpr CMDTheme M_PackTheme(uint8_t theme);
	}

	extern void M_Dye(const std::string &text,
										const cmddye::CMDTheme colors = cmddye::CMDTheme{ cmddye::CMDColor::BrightWhite },
										std::ostream &stream = std::cout);
	extern void M_Dye(const char *text,
										const cmddye::CMDTheme colors = cmddye::CMDTheme{ cmddye::CMDColor::BrightWhite },
										std::ostream &stream = std::cout);
	extern void M_Dye(const std::exception &exc,
										const cmddye::CMDTheme colors = cmddye::CMDTheme{ cmddye::CMDColor::Red },
										std::ostream &stream = std::cout);
	extern void M_EndianOrder(char *dst, const char *src, size_t length, EndianOrder order);
	extern BufferSmartPtr_t M_Hexfy(uint8_t *ptr, size_t length);
	// if padding it true, all overflow bytes are set to zero
	extern BufferVector_t M_SplitBuffer(const char *src, size_t src_len, size_t block_len, size_t *last_block_len = nullptr);

	inline const std::string &M_EndianName(const EndianOrder order)
	{
		return std::string(order == EndianOrder::Little ? "little" : "big");
	}

	inline constexpr bool M_EndianNativeToMemory(const EndianOrder order)
	{
#if defined(_WIN32) || defined(__APPLE__) || defined(__MACH__)
		return order == EndianOrder::Little;
#endif
	// unix-based
		return order == EndianOrder::Big;
	}

	inline constexpr EndianOrder M_EndianReverse(const EndianOrder order)
	{
		return (EndianOrder)(((int)order) ^ 1);
	}

	// idk why ORDER is a template.

	template <typename T, bool ORDER>
	inline T StreamReader::read()
	{
		T value{};
		load((char *)&value, sizeof(value), ORDER);
		return value;
	}

	template <typename T, bool ORDER>
	inline T StreamReader::peek()
	{
		T value{};
		load((char *)&value, sizeof(value), ORDER);
		move(-(intptr_t)sizeof(value)); // yeah
		return value;
	}

	template <typename T, bool ORDER>
	inline void StreamWriter::write(const T &value)
	{
		constexpr size_t length{ sizeof(T) };
		if (ORDER)
		{
			if (!M_EndianNativeToMemory(order())) {
				char buffer[ length ];
				M_EndianOrder(buffer, (char *)&value, length, order());
				write(buffer, length);
			}
			else
			{
				write((char *)&value, length);
			}
		}
		else
		{
			write((char *)&value, length);
		}
	}

}
