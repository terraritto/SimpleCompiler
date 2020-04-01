#pragma once
#include "GetSource.h"
#include "Table.h"
#include "CodeGen.h"

#include <memory>

constexpr int MINERROR = 3;
constexpr int FIRSTADDR = 2;

class Compiler
{
public:
	Compiler();

	void Block(int pIndex);
	void ConstDecl();
	void VarDecl();
	void FuncDecl();
	void Statement();
	void Expression();
	void Term();
	void Factor();
	void Condition();
	int IsStBeginKey(Token t);
	
	int Compile();
private:
	Token mToken;
public:
	std::shared_ptr<FileGenerator> mFile;
	std::shared_ptr<Table> mTable;
	std::shared_ptr<CodeGenerator> mCodeGenerator;
};