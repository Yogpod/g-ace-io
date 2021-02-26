#include "GarrysMod/Lua/Interface.h"
#include "Bootil/Bootil.h"
#include "bzip2/bzlib.h"

#ifdef __linux__
#include <dirent.h>
#endif

using namespace GarrysMod::Lua;

void TraverseFolder(const Bootil::BString& folder, Bootil::String::List* files, Bootil::String::List* folders) {
	// Apparently Bootil returns FULL (absolute) paths when File::Find is used on Linux. We don't want that..

#ifdef __linux__
	DIR *dir;
	struct dirent *ent;
	if ((dir = opendir(folder.c_str())) != NULL) {
		while ((ent = readdir (dir)) != NULL) {
			// Ignore symlinks etc
			if (ent->d_type == DT_REG) {
				files->push_back(ent->d_name);
			}
			else if (ent->d_type == DT_DIR && strcmp(ent->d_name, ".") != 0 && strcmp(ent->d_name, "..") != 0) {
				folders->push_back(ent->d_name);
			}
		}
		closedir (dir);
	}
#endif

#ifdef _WIN32
	// Bootil does a good job when it's Windows, so we'll just use Bootil code here
	Bootil::File::Find(files, folders, folder + "/*", false);
#endif

}

LUA_FUNCTION(LuaFunc_ListDir)
{
	const Bootil::BString& folder = LUA->CheckString(1);

	if (!Bootil::File::Exists(folder)) {
		LUA->PushBool(false);
		LUA->PushString("Folder does not exist");
		return 2;
	}

	if (!Bootil::File::IsFolder(folder)) {
		LUA->PushBool(false);
		LUA->PushString("Cannot list a non-folder");
		return 2;
	}


	Bootil::String::List files;
	Bootil::String::List folders;

	TraverseFolder(folder, &files, &folders);

	// Files
	LUA->CreateTable();
	int i = 1;

	BOOTIL_FOREACH (f, files, Bootil::String::List) {
		LUA->PushNumber(i);
		LUA->PushString((*f).c_str());
		LUA->SetTable(-3);

		i++;
	}

	// Folders
	LUA->CreateTable();
	i = 1;

	BOOTIL_FOREACH (f, folders, Bootil::String::List) {
		LUA->PushNumber(i);
		LUA->PushString((*f).c_str());
		LUA->SetTable(-3);

		i++;
	}
	
	return 2;
}

LUA_FUNCTION(LuaFunc_ReadFile)
{
	Bootil::BString out;
	if (!Bootil::File::Read(LUA->CheckString(1), out)) {
		LUA->PushBool(false);
		LUA->PushString(Bootil::Platform::LastError().c_str());
		return 2;
	}

	LUA->PushString(out.c_str(), out.length());
	return 1;
}

LUA_FUNCTION(LuaFunc_Delete)
{
	const Bootil::BString& targ = LUA->CheckString(1);

	bool success;
	if (Bootil::File::IsFolder(targ)) {
		success = Bootil::File::RemoveFolder(targ); // TODO should we recursively remove (2nd param)?
	}
	else {
		success = Bootil::File::RemoveFile(targ);
	}

	if (!success) {
		LUA->PushBool(false);
		LUA->PushString(Bootil::Platform::LastError().c_str());
		return 2;
	}

	return 0;
}

LUA_FUNCTION(LuaFunc_WriteToFile)
{
	const char* path = LUA->CheckString(1);

	LUA->CheckString(2);

	// If data string contains null bytes, the string is prematurely truncated unless we get the size explicitly
	unsigned int len;
	const char* data = LUA->GetString(2, &len);

	const Bootil::BString &strOut = std::string(data, len);

	if (!Bootil::File::Write(path, strOut)) {
		LUA->PushBool(false);
		LUA->PushString(Bootil::Platform::LastError().c_str());
		return 2;
	}

	return 0;
}

LUA_FUNCTION(LuaFunc_IsFolder)
{
	LUA->PushBool(Bootil::File::IsFolder(LUA->CheckString(1)));
	return 1;
}

LUA_FUNCTION(LuaFunc_Exists)
{
	// Bootil::File::Exists actually returns a boolean
	LUA->PushBool(Bootil::File::Exists(LUA->CheckString(1)));
	return 1;
}

LUA_FUNCTION(LuaFunc_CreateFolder)
{
	bool success = Bootil::File::CreateFolder(LUA->CheckString(1));

	if (!success) {
		LUA->PushBool(false);
		LUA->PushString(Bootil::Platform::LastError().c_str());
		return 2;
	}

	return 0;
}

int FileLastModified(const Bootil::BString& strFileName) {
	struct stat fileStat;
	int err = stat( strFileName.c_str(), &fileStat );
	if ( err != 0 ) { return 0; }
	return fileStat.st_mtime;
}


LUA_FUNCTION(LuaFunc_Time)
{
	LUA->PushNumber(FileLastModified(LUA->CheckString(1)));
	return 1;
}

LUA_FUNCTION(LuaFunc_Size)
{
	LUA->PushNumber(Bootil::File::Size(LUA->CheckString(1)));
	return 1;
}

LUA_FUNCTION(LuaFunc_CRC)
{
	LUA->PushNumber(Bootil::File::CRC(LUA->CheckString(1)));
	return 1;
}

LUA_FUNCTION(LuaFunc_BZip2)
{
	LUA->CheckString(1);

	unsigned int len;
	const char* data = LUA->GetString(1, &len);

	// This guarantees that compressed data will fit dest
	// or does it? *1.01 on an int might be too small
	double guaranteedLen = len * 1.01 + 600;

	char* dest = (char*) malloc(sizeof(char) * guaranteedLen);
	unsigned int destLen;

	int ret = BZ2_bzBuffToBuffCompress(dest, &destLen, (char*) data, len, 5, 0, 30);

	if (ret == BZ_OK) {
		LUA->PushString(dest, destLen);
		free(dest);
		return 1;
	}

	free(dest);

	LUA->PushBool(false);
	LUA->PushNumber(ret); // TODO this is lazy

	return 2;
}

LUA_FUNCTION(LuaFunc_WriteBZip2)
{
	const char* path = LUA->CheckString(1);
	LUA->CheckString(2);

	unsigned int len;
	const char* data = LUA->GetString(2, &len);

	// This guarantees that compressed data will fit dest
	// or does it? *1.01 on an int might be too small
	size_t guaranteedLen = len * 1.01 + 600;

	char* dest = (char*) malloc(sizeof(char) * guaranteedLen);
	unsigned int destLen;

	int ret = BZ2_bzBuffToBuffCompress(dest, &destLen, (char*) data, len, 5, 0, 30);

	if (ret != BZ_OK) {
		free(dest);

		LUA->PushBool(false);
		LUA->PushNumber(ret); // TODO push err string, not id?
		return 2;
	}

	const Bootil::BString &strOut = std::string(dest, destLen);
	free(dest);

	if (!Bootil::File::Write(path, strOut)) {
		LUA->PushBool(false);
		LUA->PushString(Bootil::Platform::LastError().c_str());
		return 2;
	}

	return 0;
}

#define LUA_TABLE_SET_CFUNC(name, func) \
	LUA->PushString( name ); \
	LUA->PushCFunction( func ); \
	LUA->SetTable( -3 ); \

GMOD_MODULE_OPEN()
{

	LUA->PushSpecial( GarrysMod::Lua::SPECIAL_GLOB );

	LUA->PushString( "gaceio" );
	LUA->CreateTable();

	// File IO

	LUA_TABLE_SET_CFUNC("List", LuaFunc_ListDir);
	LUA_TABLE_SET_CFUNC("Read", LuaFunc_ReadFile);
	LUA_TABLE_SET_CFUNC("Write", LuaFunc_WriteToFile);
	LUA_TABLE_SET_CFUNC("Delete", LuaFunc_Delete);
	LUA_TABLE_SET_CFUNC("IsDir", LuaFunc_IsFolder);
	LUA_TABLE_SET_CFUNC("Exists", LuaFunc_Exists);
	LUA_TABLE_SET_CFUNC("CreateDir", LuaFunc_CreateFolder);
	LUA_TABLE_SET_CFUNC("Time", LuaFunc_Time);
	LUA_TABLE_SET_CFUNC("Size", LuaFunc_Size);
	LUA_TABLE_SET_CFUNC("CRC", LuaFunc_CRC);
	
	LUA_TABLE_SET_CFUNC("BZip2", LuaFunc_BZip2);
	LUA_TABLE_SET_CFUNC("WriteBZip2", LuaFunc_WriteBZip2);

	LUA->SetTable( -3 );

	return 0;
}

//
// Called when your module is closed
//
GMOD_MODULE_CLOSE()
{
	return 0;
}
