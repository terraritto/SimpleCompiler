#pragma once
#include <string>
#include <array>

constexpr int MAXTABLE = 100; // length of name table
constexpr int MAXNAME = 31; // length of name
constexpr int MAXLEVEL = 5; // length of block

auto IsNameSize = [](std::string s) {return s.size() <= MAXNAME; };
class FileGenerator;

enum class KindT //kind of Identifier
{
	VARID,
	FUNCID,
	PARID,
	CONSTID
};

struct RelAddr //type of address, parameter, or address of function
{
	int level;
	int addr;
};

struct TableE // type of name table
{
	KindT kind; // kind of name
	std::string name; // spelling of name
	union u
	{
		int value; // const pattern:value
		struct f
		{
			RelAddr raddr; // function pattern:lead address
			int pars; // function pattern:parameter num
		};
		f funcPat; // function patern
		RelAddr raddr; // variable or parametter pattern:address
	};
	u pattern;
};

class Table
{
public:
	Table(FileGenerator* fg);
	void BlockBegin(int firstAddr); // call at start of block
	void BlockEnd(); // call at end of block
	int GetNowLevel(); // get level
	int GetNowFuncParm(); // get parameter of function
	void EnterT(std::string id); // register name at name table
	int EnterTFunc(std::string id, int v); // register function name and lead address at name table
	int EnterTVar(std::string id); // register variable name at name table
	int EnterTPar(std::string id); // register parameter name at name table
	int EnterTConst(std::string id, int v); // register const value at name table
	void EndPar(); // call at end of parameter declaration
	void ChangeV(int ti, int newVal); // change value(lead address of function) at name table 
	int SearchT(std::string id, KindT k); // return position of name table with index name: id
	KindT GetKindT(int i); // return kind of name table[i]
	RelAddr GetRelAddr(int ti); // return address of name table[ti]
	int GetVal(int ti); // return value of name table[ti]
	int GetPars(int ti); // return parameter of name table[ti]
	int FrameL(); // memory capacity that need to execute at block
public:
	static std::string kindName(KindT k); // output kind of name
private:
	FileGenerator* mFg;
	std::array<TableE, MAXTABLE> mNameTable;
	int mTableIndex;
	int mLevel; // now block level
	std::array<int, MAXLEVEL> mIndex; //index of end of block level i at index[i]
	std::array<int, MAXLEVEL> mAddress;
	int mLocalAddr;
	int mTfIndex;
};