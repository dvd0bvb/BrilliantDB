#include <iostream>
#include <cassert>
#include <filesystem>
#include "BrilliantDB.h"

using namespace BrilliantDB;

struct TestT
{
	primary_key_t i;
	foreign_key_t d;
	foreign_key_t v;
	std::string s;
};

struct TestU
{
	primary_key_t j;
	double teddy;
	int iTeddy;
};

struct TestV
{
	primary_key_t i;
	std::string s;
	std::vector<char> blob;
};

int main()
{
	try
	{
		std::filesystem::create_directory("test");

		auto DB = MakeDatabase(
			"test//test.db",
			MakeTable<TestU>("TestU",
				MakeColumn("j", &TestU::j, Constraint::primary_key, Constraint::auto_increment),
				MakeColumn("teddy", &TestU::teddy),
				MakeColumn("iTeddy", &TestU::iTeddy)
				),
			MakeTable<TestV>("TestV",
				MakeColumn("i", &TestV::i, Constraint::primary_key, Constraint::auto_increment),
				MakeColumn("s", &TestV::s),
				MakeColumn("b", &TestV::blob)
				),
			MakeTable<TestT>("TestT",
				MakeColumn("i", &TestT::i, Constraint::primary_key, Constraint::auto_increment),
				MakeColumn("d", &TestT::d),
				MakeColumn("v", &TestT::v),
				MakeColumn("s", &TestT::s),
				MakeForeignKey(&TestT::d, &TestU::j),
				MakeForeignKey(&TestT::v, &TestV::i)
				)
		);

		TestV tv{ 0,"",{'a','b','c'} };
		tv.i = DB.Insert(tv);

		TestT tt{ 0,0,0,"Test" };
		tt.i = DB.Insert(tt);

		tt.d = tv.i;
		auto ttv = DB.Get<TestV>(tt.d);

		assert(ttv);
		assert(ttv->i == tt.d);
	}
	catch (std::system_error& e)
	{
		std::cout << e.what() << '\n'
			<< e.code() << std::endl;
	}

	return 0;
}