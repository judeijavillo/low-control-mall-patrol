#ifndef __T_CU_SERIALIZER_TEST_H__
#define __T_CU_SERIALIZER_TEST_H__

namespace cugl {
	/** Main unit test that invokes all others in this module */
	void serializerUnitTest();

	void simpleTest();

	void testNumericTypes();

	void testStrings();

	void testVectors();

	void testJson();
}

#endif
