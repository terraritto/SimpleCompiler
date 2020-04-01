#pragma once
#include "Table.h"
#include "GetSource.h"

#include <array>

constexpr int MAXCODE = 200;
constexpr int MAXMEM = 2000;
constexpr int MAXREG = 20;

enum class OpCode
{
	lit, opr, lod, sto, cal, ret, ict, jmp, jpc
};

enum class Operator
{
	neg, add, sub, mul, div, odd, eq, ls, gr, neq, lseq, greq, wrt, wrl
};

struct Inst
{
	OpCode opCode;
	union Addr
	{
		RelAddr addr;
		int value;
		Operator optr;
	};
	Addr u;
};

class CodeGenerator
{
public:
	CodeGenerator(Table* table, FileGenerator* fg);
	int GenCodeV(OpCode op, int v);
	int GenCodeT(OpCode op, int ti);
	int GenCodeO(Operator p);
	int GenCodeR();

	void BackPatch(int i);
	int NextCode();
	void ListCode();
	void Execute();
	
	void CheckMax();
	void printCode(int i);
private:
	std::array<Inst, MAXCODE> mCode;
	int mCindex;
	Table* mTable;
	FileGenerator* mfile;
};