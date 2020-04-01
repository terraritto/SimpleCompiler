#include "CodeGen.h"
#include <iostream>

CodeGenerator::CodeGenerator(Table* table, FileGenerator* fg)
	: mTable(table)
	, mfile(fg)
	, mCindex(-1)
{
}

int CodeGenerator::GenCodeV(OpCode op, int v)
{
	// 命令語の生成,アドレス部:v
	CheckMax();
	mCode[mCindex].opCode = op;
	mCode[mCindex].u.value = v;
	return mCindex;
}

int CodeGenerator::GenCodeT(OpCode op, int ti)
{
	// 命令語の生成,アドレス部:名前表

	CheckMax();
	mCode[mCindex].opCode = op;
	mCode[mCindex].u.addr = mTable->GetRelAddr(ti);
	return mCindex;
}

int CodeGenerator::GenCodeO(Operator p)
{
	// 命令語の生成,アドレス部:operator
	CheckMax();
	mCode[mCindex].opCode = OpCode::opr;
	mCode[mCindex].u.optr = p;
	return mCindex;
}

int CodeGenerator::GenCodeR()
{
	// ret命令語の生成
	if (mCode[mCindex].opCode == OpCode::ret)
	{
		return mCindex; //直前生成してたら生成しない
	}
	CheckMax();
	mCode[mCindex].opCode = OpCode::ret;
	mCode[mCindex].u.addr.level = mTable->GetNowLevel();
	mCode[mCindex].u.addr.addr = mTable->GetNowFuncParm();

	return mCindex;
}

void CodeGenerator::BackPatch(int i)
{
	mCode[i].u.value = mCindex + 1;
}

int CodeGenerator::NextCode()
{
	return mCindex + 1;
}

void CodeGenerator::ListCode()
{
	int i;
	std::cout << std::endl << "code" << std::endl;
	for (int i = 0; i <= mCindex; i++)
	{
		std::cout << i << ": ";
		printCode(i);
	}
}

void CodeGenerator::Execute()
{
	int stack[MAXMEM]; //実行時スタック
	int display[MAXLEVEL]; //現在見える各ブロックの先頭番地のdisplay
	int pc, top, lev, temp;
	Inst i; //実行する命令語
	std::cout << "start execution" << std::endl;

	//top:次にスタックを入れる場所 pc:命令語のカウンタ
	top = 0; pc = 0;
	// stack[top]: calleeで壊すdisplayの退避場所
	// stack[top+1]: callerへの戻り番地
	stack[0] = 0; stack[1] = 0;
	//主ブロックの先頭番地は0
	display[0] = 0;
	
	do
	{
		i = mCode[pc++]; //これから実行する命令語

		switch (i.opCode)
		{
		case OpCode::lit:
			stack[top++] = i.u.value;
			break;
		case OpCode::lod:
			stack[top++] = stack[display[i.u.addr.level] + i.u.addr.addr];
			break;
		case OpCode::sto:
			stack[display[i.u.addr.level] + i.u.addr.addr] = stack[--top];
			break;
		case OpCode::cal:
			lev = i.u.addr.level + 1;
			// i.u.addr.level: calleeの名前のレベル
			// calleeのブロックのレベルlevはそれに+1したもの
			stack[top] = display[lev]; //display[lev]の退避
			stack[top + 1] = pc; display[lev] = top; //現在のtopがcalleeのブロックの先頭番地
			pc = i.u.addr.addr;
			break;
		case OpCode::ret:
			temp = stack[--top]; //stackのトップにあるものが返す値
			top = display[i.u.addr.level]; //topを呼ばれた時の値に戻す
			display[i.u.addr.level] = stack[top]; //壊したdisplayの回復
			pc = stack[top + 1];
			top -= i.u.addr.addr; //実引数分だけトップを戻す
			stack[top++] = temp; //返す値をstackのtopへ
			break;
		case OpCode::ict:
			top += i.u.value;
			if (top >= MAXMEM - MAXREG)
			{
				mfile->ErrorF("stack overflow");
			}
			break;
		case OpCode::jmp:
			pc = i.u.value;
			break;
		case OpCode::jpc:
			if (stack[--top] == 0)
			{
				pc = i.u.value;
			}
			break;
		case OpCode::opr:
			switch (i.u.optr)
			{
			case Operator::neg:
				stack[top - 1] = -stack[top - 1]; continue;
			case Operator::add:
				--top; stack[top - 1] += stack[top]; continue;
			case Operator::sub:
				--top; stack[top - 1] -= stack[top]; continue;
			case Operator::mul:
				--top; stack[top - 1] *= stack[top]; continue;
			case Operator::div:
				--top; stack[top - 1] /= stack[top]; continue;
			case Operator::odd:
				stack[top - 1] = stack[top - 1] & 1; continue;
			case Operator::eq:
				--top; stack[top - 1] = (stack[top-1] == stack[top]); continue;
			case Operator::ls:
				--top; stack[top - 1] = (stack[top - 1] < stack[top]); continue;
			case Operator::gr:
				--top; stack[top - 1] = (stack[top - 1] > stack[top]); continue;
			case Operator::neq:
				--top; stack[top - 1] = (stack[top - 1] != stack[top]); continue;
			case Operator::lseq:
				--top; stack[top - 1] = (stack[top - 1] <= stack[top]); continue;
			case Operator::greq:
				--top; stack[top - 1] = (stack[top - 1] >= stack[top]); continue;
			case Operator::wrt:
				std::cout << stack[--top] << " "; continue;
			case Operator::wrl:
				std::cout << std::endl; continue;
			}
		}
	} while (pc != 0);
}

void CodeGenerator::CheckMax()
{
	if (++mCindex < MAXCODE)
	{
		return;
	}
	mfile->ErrorF("too many code");
}

void CodeGenerator::printCode(int i)
{
	int flag;
	
	switch (mCode[i].opCode)
	{
	case OpCode::lit:
		std::cout << "lit"; flag = 1; break;
	case OpCode::opr:
		std::cout << "opr"; flag = 3; break;
	case OpCode::lod:
		std::cout << "lod"; flag = 2; break;
	case OpCode::sto:
		std::cout << "sto"; flag = 2; break;
	case OpCode::cal:
		std::cout << "cal"; flag = 2; break;
	case OpCode::ret:
		std::cout << "ret"; flag = 2; break;
	case OpCode::ict:
		std::cout << "ict"; flag = 1; break;
	case OpCode::jmp:
		std::cout << "jmp"; flag = 1; break;
	case OpCode::jpc:
		std::cout << "jpc"; flag = 1; break;
	}

	switch (flag)
	{
	case 1:
		std::cout << "," << mCode[i].u.value << std::endl;
		return;
	case 2:
		std::cout << "," << mCode[i].u.addr.level;
		std::cout << "," << mCode[i].u.addr.addr << std::endl;
		return;
	case 3:
		switch (mCode[i].u.optr)
		{
		case Operator::neg:
			std::cout << ",neg" << std::endl; return;
		case Operator::add:
			std::cout << ",add" << std::endl; return;
		case Operator::sub:
			std::cout << ",sub" << std::endl; return;
		case Operator::mul:
			std::cout << ",mul" << std::endl; return;
		case Operator::div:
			std::cout << ",div" << std::endl; return;
		case Operator::odd:
			std::cout << ",odd" << std::endl; return;
		case Operator::eq:
			std::cout << ",eq" << std::endl; return;
		case Operator::ls:
			std::cout << ",ls" << std::endl; return;
		case Operator::gr:
			std::cout << ",gr" << std::endl; return;
		case Operator::neq:
			std::cout << ",neq" << std::endl; return;
		case Operator::lseq:
			std::cout << ",lseq" << std::endl; return;
		case Operator::greq:
			std::cout << ",greq" << std::endl; return;
		case Operator::wrt:
			std::cout << ",wrt" << std::endl; return;
		case Operator::wrl:
			std::cout << ",wrl" << std::endl; return;
		}
	}
}
