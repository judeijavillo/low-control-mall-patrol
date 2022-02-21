#include "TCUSerializerTest.h"

#include <cugl/cugl.h>
#include <cugl/net/CUNetworkSerializer.h>

void cugl::serializerUnitTest() {
	cugl::simpleTest();
	cugl::testNumericTypes();
	cugl::testStrings();
	cugl::testVectors();
	cugl::testJson();
}

void cugl::simpleTest() {
	cugl::NetworkSerializer test;
	test.write("hello world");
	test.write(-123.4);
	test.write((int64_t)5);
	std::vector<std::string> testV = { "hi" };
	test.write(testV);

	std::vector<uint8_t> d(test.serialize());

	cugl::NetworkDeserializer test2;
	test2.receive(d);

	CUAssertAlwaysLog(std::get<std::string>(test2.read()) == "hello world", "string test");
	CUAssertAlwaysLog(std::get<double>(test2.read()) == -123.4, "double test");
	CUAssertAlwaysLog(std::get<int64_t>(test2.read()) == 5, "int test");
	auto vv = std::get<std::vector<std::string>>(test2.read());
	CUAssertAlwaysLog(vv.size() == 1, "vector test");
	CUAssertAlwaysLog(vv[0] == "hi", "vector test");
}

void cugl::testNumericTypes() {
	std::vector<uint32_t> u32 = {
		0,1,2,3,4,5,13092285,
		std::numeric_limits<uint32_t>::min(), std::numeric_limits<uint32_t>::max()
	};
	std::vector<int32_t> s32 = {
		-1,0,1,2,3,4,5,10,234523423,std::numeric_limits<int32_t>::min(),std::numeric_limits<int32_t>::max()
	};
	std::vector<uint64_t> u64 = {
		0,1,2,3,4,5,13092285,std::numeric_limits<uint64_t>::min(),std::numeric_limits<uint64_t>::max()
	};
	std::vector<int64_t> s64 = {
		-1,0,1,2,3,4,5,10,234523423,std::numeric_limits<int64_t>::min(),std::numeric_limits<int64_t>::max()
	};
	std::vector<float> f = {
		-1,0,1,2,3,4,1.1f,0.1f,2324.23423f,-23422,4393,-34534.3453f,-0.000001f,
		std::numeric_limits<float>::min(),
		std::numeric_limits<float>::max(),
		std::numeric_limits<float>::lowest(),
		INFINITY, -INFINITY
	};
	std::vector<double> d = {
		-1,0,1,2,3,4,1.1,0.1,2324.23423,-23422,4393,-34534.3453,-0.0000001,
		std::numeric_limits<double>::min(),
		std::numeric_limits<double>::max(),
		std::numeric_limits<double>::lowest(),
		std::numeric_limits<double>::infinity(), -std::numeric_limits<double>::infinity()
	};

	cugl::NetworkSerializer test;
	for (auto& e : u32) {
		test.write(e);
	}
	for (auto& e : s32) {
		test.write(e);
	}
	for (auto& e : u64) {
		test.write(e);
	}
	for (auto& e : s64) {
		test.write(e);
	}
	for (auto& e : f) {
		test.write(e);
	}
	for (auto& e : d) {
		test.write(e);
	}

	std::vector<uint8_t> dd(test.serialize());
	cugl::NetworkDeserializer test2;
	test2.receive(dd);
	for (auto& e : u32) {
		CUAssertAlwaysLog(e == std::get<uint32_t>(test2.read()), "uint32 test");
	}
	for (auto& e : s32) {
		CUAssertAlwaysLog(e == std::get<int32_t>(test2.read()), "int32 test");
	}
	for (auto& e : u64) {
		CUAssertAlwaysLog(e == std::get<uint64_t>(test2.read()), "uint64 test");
	}
	for (auto& e : s64) {
		CUAssertAlwaysLog(e == std::get<int64_t>(test2.read()), "int64 test");
	}
	for (auto& e : f) {
		CUAssertAlwaysLog(e == std::get<float>(test2.read()), "float test");
	}
	for (auto& e : d) {
		CUAssertAlwaysLog(e == std::get<double>(test2.read()), "double test");
	}
}

void cugl::testStrings() {
	std::vector<std::string> tests = {
		"hello world", "ABCdefg", "", "e984892fjp;aw4980t49p8hht3w\n\nw4wer\t\t98wr98h894",
		"OIEOIRH$)(hrwhtWH$(H(HT$*(YHRH92)(RU#**(YHRT(*#(T$twert934whiureyif9f\x00vvdi"
	};

	cugl::NetworkSerializer test;
	for (auto& e : tests) {
		test.write(e);
	}
	std::vector<uint8_t> dd(test.serialize());
	cugl::NetworkDeserializer test2;
	test2.receive(dd);
	for (auto& e : tests) {
		CUAssertAlwaysLog(e == std::get<std::string>(test2.read()), "string test");
	}
}

void cugl::testVectors() {
	std::vector<std::vector<float>> fv = {
		{1.0f, 0.0f, 2.1f, 1.33f},
		{2.0f, 0.0f, -2.0f, 193.f},
		{9999.f, 0.001f,2234.f, 0.0f, 1.0f}
	};
	std::vector<std::vector<std::string>> fs = {
		{"hi", "bye", "boo"},
		{"1", "", "092530e9w(*H(*H(*"},
		{},
		{"2340jr09828930hjr892hr9823h98r2h98r29r34"}
	};

	cugl::NetworkSerializer test;
	for (auto& e : fv) {
		test.write(e);
	}
	for (auto& e : fs) {
		test.write(e);
	}

	std::vector<uint8_t> dd(test.serialize());
	cugl::NetworkDeserializer test2;
	test2.receive(dd);
	for (auto& e : fv) {
		CUAssertAlwaysLog(e == std::get < std::vector<float> > (test2.read()), "float vector test");
	}
	for (auto& e : fs) {
		CUAssertAlwaysLog(e == std::get < std::vector<std::string> >(test2.read()), "string vector test");
	}
}

void cugl::testJson() {
	cugl::JsonValue v;
	v.initWithJson("{\"a\":1.222,\"b\":true,\"c\":false,\"d\":null,\"e\":[1,2,3],\"f\":[1,2,\"false\",true,null],\"g\":{\"zzz\":1,\"xxx\":\"why\",\"yyy\":true,\"www\":null,\"aaa\":[1,2,3,false]},\"h\":\"hello world this is an annoying json\"}");
	
	cugl::NetworkSerializer test;
	test.write(std::make_shared<cugl::JsonValue>(v));
	std::vector<uint8_t> dd(test.serialize());
	cugl::NetworkDeserializer test2;
	test2.receive(dd);

	// This test is quite fragile because there's no guarantees JSONs stringify in the same key order
	CUAssertAlwaysLog(v.toString() == std::get < std::shared_ptr<cugl::JsonValue> >(test2.read())->toString(), "Json test");
}
