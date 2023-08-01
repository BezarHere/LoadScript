#pragma once
#include <iostream>
#include <vector>
#include <unordered_map>
#include "Bite.h"

namespace loadscript
{
	class NodeData;
	typedef uint64_t hash_t;
	typedef std::unordered_map<hash_t, NodeData> Table_t;
	typedef std::vector<NodeData> Array_t;

	class ScriptMetadata
	{
	public:
		ScriptMetadata(const std::string &str, const bool srcStr = false);
		
		ScriptMetadata(const ScriptMetadata &copy);
		ScriptMetadata operator=(const ScriptMetadata &copy);

		void invalidateSourceCache();

		bool getAlwaysLoadSource() const;
		void setAlwaysLoadSource(bool value);

		void reloadSource();

		bool sourceUpdatePending() const;

		const std::string &getSource();
		// will throw if sourceUpdatePending() is true
		const std::string &getSource() const;

		const std::string &getPath() const;


	private:
		std::string m_str;
		std::string m_src;
		bool m_cachedSource{ false };
		bool m_alwaysLoadSource{ false }; // e.g. no caching
		const bool m_srcStr{ false };
	};

	enum class SyntaxNodeType : uint16_t;
	enum class NodeDataConvState: uint8_t;

	enum class ErrorCode : uint16_t
	{
		Ok,
		Failed,
		ExpectedExpression,
		UnexpectedIdentifier,
		ExpectedOperator,
		UnexpectedSymbol,
		InvalidIndentation,
		UncompatibleTypes,
		UnexpectedEOF,
	};

	struct Error
	{
		ErrorCode code;
		const char *what = nullptr;
		uint32_t line = (uint32_t)-1;
	};

	// for bitmasking stuff, this can't go above 15U
	// but where safe with 6U
	enum class NodeDataType : uint8_t
	{
		None,
		Byte,
		Int,
		Float,
		String, // can be an identifier too
		Table, // std::unordered_map<hash_t, NodeData>
		Array, // std::vector<NodeData>
		Max
	};

	static constexpr size_t _NodeDataUnion_size = 80;
	union NodeDataUnion
	{
		uint8_t byte;
		int64_t int_;
		float float_;
		std::string str;
		Table_t table;
		Array_t arr;
		uint8_t mem[ _NodeDataUnion_size ];

		__forceinline NodeDataUnion()
			: mem{}
		{
		}

		NodeDataUnion(const NodeDataUnion &copy);
	

		__forceinline ~NodeDataUnion()
		{
		}

		void clear();

	};

	class NodeData
	{
	public:
		NodeData();
		NodeData(NodeDataType type);
		NodeData(const NodeData &copy);

		void operator=(const NodeData &other);

		NodeData(uint8_t value);
		NodeData(int64_t value);
		NodeData(float value);
		NodeData(const std::string &value);
		NodeData(const Table_t &value);
		NodeData(const Array_t &value);

		void set(uint8_t value);
		void set(int64_t value);
		void set(float value);
		void set(const std::string &value);
		void set(const Table_t &value);
		void set(const Array_t &value);

		NodeDataUnion &get();
		const NodeDataUnion &get() const;

		NodeDataType getType() const;
		// if you want to set the underlaying type, but if you
		// want to convert the type then use 'convert'
		// use this if your gonna reset the value
		void setType(NodeDataType type);
		void convert(NodeDataType type);

		void encode(bite::StreamWriter &writer) const;
		void decode(bite::StreamReader &reader);

	private:
		NodeDataUnion m_data;
		NodeDataType m_type{ NodeDataType::Int };
		// TODO: implement static typing
		bool m_static{ false };
	};

	struct SyntaxNode
	{
		SyntaxNodeType type{ (SyntaxNodeType)0U };
		std::unique_ptr<NodeData> data{};
		SyntaxNode *parent;
		std::vector<SyntaxNode> children;
	};

	class LoadScriptModule
	{

	};

	class LoadScript
	{
	public:
		LoadScript(const ScriptMetadata &metadata);

		ScriptMetadata &getMetadata();
		const ScriptMetadata &getMetadata() const;
		void setMetadata(const ScriptMetadata &metadata);

	private:
		ScriptMetadata m_meta;
	};

	class ScriptEnviorment
	{

		bool m_debug{ true };
		std::vector<std::shared_ptr<LoadScriptModule>> m_modules;
		std::unordered_map<size_t, LoadScript> m_scripts;
	};


}
