#include "pch.h"
#include "LoadScript.h"

#define out
#define forceinline __forceinline

using bite::BufferVector_t;
using bite::StreamReader;
using bite::StreamWriter;
using namespace loadscript;

typedef struct
{
	Error error;
	std::vector<std::string> warnings;
} Report_t;

enum class WordTokenType : uint8_t
{

};

struct WordToken
{
	forceinline WordToken(const std::string &str)
		: text(str)
	{}

	std::string text;
	WordTokenType type;
};
typedef std::vector<WordToken> TokenBuffer_t;

forceinline std::string f_LexerSrc_ExtractStr(const char *const cstr, const size_t len)
{
	// base case
	if (len <= 2)
		return std::string(cstr, len);
	const char str_char = cstr[ 0 ];
	std::istringstream stream{};

	size_t i{};
	size_t j{ 1 };
	while (i++ < len)
	{
		if (cstr[ i ] == '\\')
		{
			char ec = cstr[ ++i ];
		}
	}

}

forceinline Error f_LexerSrc(const std::string &str, const std::string &path, TokenBuffer_t tokens)
{
#define word(c) (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || c == '_' || (c >= '0' && c <= '9')
	//static const std::regex breaker{ "\\d+(\\.\\d+)?|[A-Za-z_]\\w+|\n|\t+| +|.", std::regex::icase | std::regex::optimize };
	char c{};
	size_t j{};
	const char *const cstr = str.c_str();
	const size_t size = str.size();
	for (size_t i{}; i < size; i++)
	{
		j = i;
		c = cstr[ i ];

		// strings
		if (c == '"' || c == '\'')
		{
			tokens.emplace_back(f_LexerSrc_ExtractStr(cstr + i, size - i));
		}
		// identifiers
		else if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || c == '_')
		{
			while (++i < size && word(cstr[ i ]));
			tokens.emplace_back(std::string(cstr + j, cstr + i));
		}
		// numpers
		else if ((c >= '0' && c <= '9'))
		{
			bool doted{ false };
			while (++i < size)
			{
				if ((cstr[ i ] >= '0' && cstr[ i ] <= '9'));
				else if (!doted && cstr[ i ] == '.') doted = true;
				else break;
			}
			tokens.emplace_back(std::string(cstr + j, cstr + i));
		}
		// identation
		else if (c == '\t')
		{
			while (++i < size && cstr[ i ] == '\t');
			tokens.emplace_back(std::string(cstr + j, cstr + i));
		}
		// new lines\spaces
		else if (c == '\n' || c == ' ')
		{
			// skiping all new lines\spaces except the first one
			while (++i < size && cstr[ i ] == c);
			tokens.emplace_back(std::string(cstr + j, cstr + j + 1));
		}
		// double operators (==, <<)
		else if (c == cstr[ i + 1 ])
			tokens.emplace_back(std::string(cstr + i, cstr + i + 2));
		// other
		else tokens.emplace_back(std::string(cstr + i, cstr + i + 1));
	}
	for (const auto &i : tokens)
	{
		std::cout << i.text << '\n';
	}
	return {ErrorCode::Ok};
#undef word
}

forceinline Report_t f_parseScript(ScriptMetadata &meta, out LoadScript &script)
{
	// idk if this is an overkill or not, but it's tiny compared to modren memories
	constexpr size_t TokenBufferReservationCapacity = 8192;
	Report_t report{};
	const std::string &src{ meta.getSource() };

	std::istringstream sstream{ src };
	TokenBuffer_t tokens_buffer;

	tokens_buffer.reserve(TokenBufferReservationCapacity);

	f_LexerSrc(src, meta.getPath(), tokens_buffer);

	return report;
}

forceinline void f_encodeSyntaxNode_Write(const SyntaxNode &node, StreamWriter &writer)
{
	writer.write((uint16_t)node.type);
	if (node.data)
		node.data->encode(writer);
}

// encode_tree will encode all the node's children
forceinline void f_encodeSyntaxNode(const SyntaxNode &node, StreamWriter &writer, const bool encode_tree = true)
{
	f_encodeSyntaxNode_Write(node, writer);
	if (encode_tree)
	{
		writer.write<int32_t>((int32_t)node.children.size());
		for (const SyntaxNode &p : node.children)
		{
			f_encodeSyntaxNode(p, writer, encode_tree);
		}
	}
	else
	{
		writer.write<int32_t>(-1);
	}
}

forceinline constexpr NodeDataConvState f_getConvState(NodeDataType from, NodeDataType to)
{
	constexpr uint8_t MAX_TYPE = (uint8_t)NodeDataType::Max;
	return (NodeDataConvState)(((uint8_t)from * MAX_TYPE) + (uint8_t)to);
}

namespace loadscript
{
	enum class SyntaxNodeType : uint16_t
	{
		None = 0,
		Root,
		Variable,
		Identifier,
		LocalVariable,
		Function,
		LocalFunction,
		Require, // load another script
		Call, // first child is the function's name, other children are the paramters
		Create,
		//Raise, //!!! NO EXCEPTION HANDLING !!!
		// Built-in Types
		BT_Int,
		BT_Byte,
		BT_Float,
		BT_Double,
		BT_String,
		BT_Table,
		BT_Array,
		BT_Function,
		// Control Flow
		CF_If,
		CF_Elif,
		CF_Else,
		CF_For,
		CF_While,
		CF_Return,
		CF_Jump,
		CF_Break,
		CF_Continue,
		// Operators (binary)
		OP_Assigen,
		OP_Add,
		OP_Sub,
		OP_Mul,
		OP_Div,
		OP_Mod,
		OP_AND,
		OP_OR,
		OP_Equal,
		OP_NotEqual,
		OP_Acsses, // the '.'
		// unary
		OP_Negate,
		// Built-in Functions
		BM_Print,
		BM_Input,
		BM_Sin,
		BM_Cosin,
		BM_Tan,
		BM_Atan2,
		// Global variables
		GV_SYS,
		GV_Cpp
	};

	// no good. wtf? why i did this?
	// must A to A are only to not offset the lookup
	enum class NodeDataConvState : uint8_t
	{
		NoneToNone,
		NoneToByte,
		NoneToInt,
		NoneToFloat,
		NoneToString,
		NoneToTable,
		NoneToArray,
		ByteToNone,
		ByteToByte,
		ByteToInt,
		ByteToFloat,
		ByteToString,
		ByteToTable,
		ByteToArray,
		IntToNone,
		IntToByte,
		IntToInt,
		IntToFloat,
		IntToString,
		IntToTable,
		IntToArray,
		FloatToNone,
		FloatToByte,
		FloatToInt,
		FloatToFloat,
		FloatToString,
		FloatToTable,
		FloatToArray,
		StringToNone,
		StringToByte,
		StringToInt,
		StringToFloat,
		StringToString,
		StringToTable,
		StringToArray,
		TableToNone,
		TableToByte,
		TableToInt,
		TableToFloat,
		TableToString,
		TableToTable,
		TableToArray,
		ArrayToNone,
		ArrayToByte,
		ArrayToInt,
		ArrayToFloat,
		ArrayToString,
		ArrayToTable,
		ArrayToArray
	};

	ScriptMetadata::ScriptMetadata(const std::string &str, const bool strStr)
		: m_str(str), m_srcStr(strStr)
	{
	}

	ScriptMetadata::ScriptMetadata(const ScriptMetadata &copy)
		: m_str(copy.m_str), m_src(copy.m_src),
		m_alwaysLoadSource(copy.m_alwaysLoadSource), m_cachedSource(copy.m_cachedSource),
		m_srcStr(copy.m_srcStr)
	{
	}

	ScriptMetadata ScriptMetadata::operator=(const ScriptMetadata &copy)
	{
		return ScriptMetadata(copy);
	}

	void ScriptMetadata::invalidateSourceCache()
	{
		m_cachedSource = false;
	}

	bool ScriptMetadata::getAlwaysLoadSource() const
	{
		return m_alwaysLoadSource;
	}

	void ScriptMetadata::setAlwaysLoadSource(bool value)
	{
		if (value == m_alwaysLoadSource)
			return;
		m_alwaysLoadSource = value;
		if (!value)
			invalidateSourceCache();
	}

	void ScriptMetadata::reloadSource()
	{
		if (m_srcStr)
		{
			m_src = m_str;
			return;
		}
		StreamReader reader{ m_str, bite::EndianOrder::Little };
		m_src = std::string(reader.readCstr(reader.size()).release());
		m_cachedSource = true;
	}

	bool ScriptMetadata::sourceUpdatePending() const
	{
		return m_alwaysLoadSource || !m_cachedSource;
	}

	const std::string &ScriptMetadata::getSource()
	{
		if (sourceUpdatePending())
			reloadSource();
		return m_src;
	}

	const std::string &ScriptMetadata::getSource() const
	{
		if (sourceUpdatePending())
			throw std::runtime_error("Source needs to be updated but the script metadata is in a const state");
		return m_src;
	}

	const std::string &ScriptMetadata::getPath() const
	{
		return m_str;
	}

	LoadScript::LoadScript(const ScriptMetadata &metadata)
		: m_meta{ metadata }
	{
		f_parseScript(m_meta, *this);
	}

	ScriptMetadata &LoadScript::getMetadata()
	{
		return m_meta;
	}

	const ScriptMetadata &LoadScript::getMetadata() const
	{
		return m_meta;
	}

	void LoadScript::setMetadata(const ScriptMetadata &metadata)
	{
		m_meta = metadata;
	}

#pragma region(Node data)

	NodeData::NodeData()
		: NodeData(NodeDataType::None)
	{
	}

	NodeData::NodeData(NodeDataType type)
		: m_type{ type }
	{
	}

	NodeData::NodeData(const NodeData &copy)
		: m_type(copy.m_type), m_data(copy.m_data)
	{
	}

	void NodeData::operator=(const NodeData &other)
	{
		static_assert(sizeof(NodeDataUnion) == _NodeDataUnion_size);
		m_type = other.m_type;
		memcpy(&m_data, &other.m_data, _NodeDataUnion_size);
	}

	NodeData::NodeData(uint8_t value)
		: NodeData(NodeDataType::Byte)
	{
		m_data.byte = value;
	}


	NodeData::NodeData(int64_t value)
		: NodeData(NodeDataType::Int)
	{
		m_data.int_ = value;
	}

	NodeData::NodeData(float value)
		: NodeData(NodeDataType::Float)
	{
		m_data.float_ = value;
	}

	NodeData::NodeData(const std::string &value)
		: NodeData(NodeDataType::String)
	{
		m_data.str = value;
	}

	NodeData::NodeData(const Table_t &value)
		: NodeData(NodeDataType::Table)
	{
		m_data.table = value;
	}

	NodeData::NodeData(const Array_t &value)
		: NodeData(NodeDataType::Array)
	{
		m_data.arr = value;
	}

	void NodeData::set(uint8_t value)
	{
		m_data.byte = value;
	}

	void NodeData::set(int64_t value)
	{
		m_data.int_ = value;
	}

	void NodeData::set(float value)
	{
		m_data.float_ = value;
	}

	void NodeData::set(const std::string &value)
	{
		m_data.str = value;
	}

	void NodeData::set(const Table_t &value)
	{
		m_data.table = value;
	}

	void NodeData::set(const Array_t &value)
	{
		m_data.arr = value;
	}

	NodeDataUnion &NodeData::get()
	{
		return m_data;
	}

	const NodeDataUnion &NodeData::get() const
	{
		return m_data;
	}

	NodeDataType NodeData::getType() const
	{
		return m_type;
	}

	void NodeData::setType(NodeDataType type)
	{
		m_type = type;
	}

	void NodeData::convert(NodeDataType type)
	{
		// can't convert to same type
		if (m_type == type)
			return;
		// converting to None
		if (type == NodeDataType::None)
		{
			m_data.clear();
			setType(type);
			return;
		}
		else if (m_type == NodeDataType::None) // converting from None
		{
			setType(type);
			return;
		}
		switch (f_getConvState(getType(), type))
		{
			case NodeDataConvState::ByteToFloat:
				m_data.float_ = (float)m_data.byte;
				break;
			case NodeDataConvState::IntToFloat:
				m_data.float_ = (float)m_data.int_;
				break;
			case NodeDataConvState::FloatToByte:
				m_data.byte = (uint8_t)m_data.float_;
				break;
			case NodeDataConvState::FloatToInt:
				m_data.int_ = (int64_t)m_data.float_;
				break;
			default:
				break;
		}


		setType(type);
	}

	void NodeData::encode(StreamWriter &writer) const
	{
	}

	void NodeData::decode(StreamReader &reader)
	{

	}
#pragma endregion

	NodeDataUnion::NodeDataUnion(const NodeDataUnion &copy)
		: mem{}
	{
		static_assert(sizeof(NodeDataUnion) == _NodeDataUnion_size);
		memcpy(mem, copy.mem, _NodeDataUnion_size);
	}

	void NodeDataUnion::clear()
	{
		static_assert(sizeof(NodeDataUnion) == _NodeDataUnion_size);
		memset(this, 0, sizeof(NodeDataUnion));
	}

}

