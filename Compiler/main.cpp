#include "Compile.h"
#include <string>

int main()
{
	Compiler obj;
	std::string fileName = "test.txt";
	
	if (!obj.mFile->OpenSource(fileName)) {
		return 0;
	}
	if (obj.Compile())
	{
		obj.mCodeGenerator->Execute();
	}
	obj.mFile->CloseSource();
}