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
	// ���ߌ�̐���,�A�h���X��:v
	CheckMax();
	mCode[mCindex].opCode = op;
	mCode[mCindex].u.value = v;
	return mCindex;
}

int CodeGenerator::GenCodeT(OpCode op, int ti)
{
	// ���ߌ�̐���,�A�h���X��:���O�\

	CheckMax();
	mCode[mCindex].opCode = op;
	mCode[mCindex].u.addr = mTable->GetRelAddr(ti);
	return mCindex;
}

int CodeGenerator::GenCodeO(Operator p)
{
	// ���ߌ�̐���,�A�h���X��:operator
	CheckMax();
	mCode[mCindex].opCode = OpCode::opr;
	mCode[mCindex].u.optr = p;
	return mCindex;
}

int CodeGenerator::GenCodeR()
{
	// ret���ߌ�̐���
	if (mCode[mCindex].opCode == OpCode::ret)
	{
		return mCindex; //���O�������Ă��琶�����Ȃ�
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
	int stack[MAXMEM]; //���s���X�^�b�N
	int display[MAXLEVEL]; //���݌�����e�u���b�N�̐擪�Ԓn��display
	int pc, top, lev, temp;
	Inst i; //���s���閽�ߌ�
	std::cout << "start execution" << std::endl;

	//top:���ɃX�^�b�N������ꏊ pc:���ߌ�̃J�E���^
	top = 0; pc = 0;
	// stack[top]: callee�ŉ�display�̑ޔ��ꏊ
	// stack[top+1]: caller�ւ̖߂�Ԓn
	stack[0] = 0; stack[1] = 0;
	//��u���b�N�̐擪�Ԓn��0
	display[0] = 0;
	
	do
	{
		i = mCode[pc++]; //���ꂩ����s���閽�ߌ�

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
			// i.u.addr.level: callee�̖��O�̃��x��
			// callee�̃u���b�N�̃��x��lev�͂����+1��������
			stack[top] = display[lev]; //display[lev]�̑ޔ�
			stack[top + 1] = pc; display[lev] = top; //���݂�top��callee�̃u���b�N�̐擪�Ԓn
			pc = i.u.addr.addr;
			break;
		case OpCode::ret:
			temp = stack[--top]; //stack�̃g�b�v�ɂ�����̂��Ԃ��l
			top = display[i.u.addr.level]; //top���Ă΂ꂽ���̒l�ɖ߂�
			display[i.u.addr.level] = stack[top]; //�󂵂�display�̉�
			pc = stack[top + 1];
			top -= i.u.addr.addr; //�������������g�b�v��߂�
			stack[top++] = temp; //�Ԃ��l��stack��top��
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
