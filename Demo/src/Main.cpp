#include "pch.h"

using namespace loadscript;

int main(int argc, char **argv)
{
	const std::string p{ "E:\\Assets\\visual studio\\LoadScript\\Demo\\src\\" };
	ScriptMetadata meta{ p + "test.ls" };
	LoadScript script{ meta };
}


