#include "Table.h"
#include "GetSource.h"
#include <iostream>
#include <algorithm>

Table::Table(FileGenerator* fg)
	: mTableIndex(0)
	, mLevel(-1)
	, mLocalAddr(0)
	, mTfIndex(0)
	, mFg(fg)
{
}

void Table::BlockBegin(int firstAddr)
{
	if (mLevel == -1) // init
	{
		mLocalAddr = firstAddr;
		mTableIndex = 0;
		mLevel++;
		return;
	}

	if (mLevel == MAXLEVEL - 1)
	{
		mFg->ErrorF("too many nested blocks");
		return;
	}

	mIndex[mLevel] = mTableIndex; // set now block info
	mAddress[mLevel] = mLocalAddr;
	mLocalAddr = firstAddr; // set new block addr
	mLevel++; // new block level
	return;
}

void Table::BlockEnd()
{
	mLevel--; // down levele
	mTableIndex = mIndex[mLevel >= 0 ? mLevel :0]; // recover info
	mLocalAddr = mAddress[mLevel >= 0 ? mLevel : 0];
}

int Table::GetNowLevel()
{
	return mLevel;
}

int Table::GetNowFuncParm()
{
	return mNameTable.at(mIndex[mLevel-1 >=0 ? mLevel-1:0]).pattern.funcPat.pars; // return num of function parameter
}

void Table::EnterT(std::string id)
{
	if (++mTableIndex < MAXTABLE)
	{
		mNameTable[mTableIndex].name = id;
	}
	else
	{
		mFg->ErrorF("too many names");
	}
}

int Table::EnterTFunc(std::string id, int v)
{
	EnterT(id);
	mNameTable.at(mTableIndex).kind = KindT::FUNCID;
	mNameTable.at(mTableIndex).pattern.funcPat.raddr.level = mLevel;
	mNameTable.at(mTableIndex).pattern.funcPat.raddr.addr = v; // set lead address of func
	mNameTable.at(mTableIndex).pattern.funcPat.pars = 0; // initial of parameter num
	mTfIndex = mTableIndex;
	return mTableIndex;
}

int Table::EnterTVar(std::string id)
{
	EnterT(id);
	mNameTable[mTableIndex].kind = KindT::VARID;
	mNameTable[mTableIndex].pattern.raddr.level = mLevel;
	mNameTable[mTableIndex].pattern.raddr.addr = mLocalAddr++;
	return mTableIndex;
}

int Table::EnterTPar(std::string id)
{
	EnterT(id);
	mNameTable[mTableIndex].kind = KindT::PARID;
	mNameTable[mTableIndex].pattern.raddr.level = mLevel;
	mNameTable[mTfIndex].pattern.funcPat.pars++;
	return mTableIndex;
}

int Table::EnterTConst(std::string id, int v)
{
	EnterT(id);
	mNameTable[mTableIndex].kind = KindT::CONSTID;
	mNameTable[mTableIndex].pattern.value = v;
	return mTableIndex;
}

void Table::EndPar()
{
	int pars = mNameTable[mTfIndex].pattern.funcPat.pars;
	if (pars == 0) { return; }
	for (int i = 1; i <= pars; i++)
	{
		// calculate address for each parameter
		mNameTable[mTfIndex + i].pattern.raddr.addr = i - 1 - pars;
	}
}

void Table::ChangeV(int ti, int newVal)
{
	mNameTable[ti].pattern.funcPat.raddr.addr = newVal; // change addrss
}

int Table::SearchT(std::string id, KindT k)
{
	auto result = std::find_if(
		mNameTable.begin(),
		mNameTable.end(),
		[id](TableE temp) {return temp.name == id; }
	);

	if (result != mNameTable.end())
	{
		return std::distance(mNameTable.begin(), result);
	}
	else
	{
		mFg->ErrorF("undef");
		if (k == KindT::VARID) { return EnterTVar(id); } // if type is var, it regist
		return 0;
	}
}

KindT Table::GetKindT(int i)
{
	return mNameTable[i].kind;
}

RelAddr Table::GetRelAddr(int ti)
{
	return mNameTable[ti].pattern.raddr;
}

int Table::GetVal(int ti)
{
	return mNameTable[ti].pattern.value;
}

int Table::GetPars(int ti)
{
	return mNameTable[ti].pattern.funcPat.pars;
}

int Table::FrameL()
{
	return mLocalAddr;
}

std::string Table::kindName(KindT k)
{
	switch (k)
	{
	case KindT::VARID:
		return "var";
		break;
	case KindT::PARID:
		return "par";
		break;
	case KindT::FUNCID:
		return "func";
		break;
	case KindT::CONSTID:
		return "const";
		break;
	default:
		break;
	}
}
