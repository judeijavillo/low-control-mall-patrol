//
//  CUNetworkSerializer.h
//  Cornell University Game Library (CUGL)
//
//  This module provides support for serializing and deserializing information
//  across the network. The class NetworkConnection can only handle byte arrays.
//  These classes allow you to transfer much more complex data into these byte
//  arrays.
//
//  These classes do not QUITE use our standard shared-pointer architecture. That
//  is because there is no non-trivial constructor patterns and so an init method
//  is not necessary. However, we do still include an alloc method for creating
//  shared pointers.
//
//  CUGL MIT License:
//      This software is provided 'as-is', without any express or implied
//      warranty.  In no event will the authors be held liable for any damages
//      arising from the use of this software.
//
//      Permission is granted to anyone to use this software for any purpose,
//      including commercial applications, and to alter it and redistribute it
//      freely, subject to the following restrictions:
//
//      1. The origin of this software must not be misrepresented; you must not
//      claim that you wrote the original software. If you use this software
//      in a product, an acknowledgment in the product documentation would be
//      appreciated but is not required.
//
//      2. Altered source versions must be plainly marked as such, and must not
//      be misrepresented as being the original software.
//
//      3. This notice may not be removed or altered from any source distribution.
//
// Author: Michael Xing
// Version: 5/25/2021
//
// Minor compatibility edits by Walker White (2/2/22).
// 
// With help from onewordstudios:
// - Demi Chang
// - Aashna Saxena
// - Sam Sorenson
// - Michael Xing
// - Jeffrey Yao
// - Wendy Zhang
// https://onewordstudios.com/
//
// With thanks to the students of CS 4152 Spring 2021 for beta testing this class.
//
#ifndef __CU_NETWORK_SERIALIZER_H__
#define __CU_NETWORK_SERIALIZER_H__

#include <variant>
#include <string>
#include <vector>
#include <cstdint>
#include <cugl/assets/CUJsonValue.h>

namespace cugl {

/**
 * Represents the type of the of a serialized value.
 *
 * Whenever you write a value to {@link NetworkSerializer}, it is prefixed by
 * a message type indicating what has been encoded. You should use this enum in 
 * conjunction with {@link NetworkDeserializer} to determine the next value to 
 * read.
 */
enum NetworkType : uint8_t {
    /** Represents null in jsons */
    NoneType,
    /** 
     * Represents a true boolean value 
     *
     * In order to minimize data transfer, booleans are encoded directly into
     * their message header.
     */
    BooleanTrue,
    /** 
     * Represents a false boolean value 
     *
     * In order to minimize data transfer, booleans are encoded directly into
     * their message header.
     */
    BooleanFalse,
    /** Represents a float value */
    FloatType,
    /** Represents a double value */
    DoubleType,
    /** Represents an unsigned 32 bit int */
    UInt32Type,
    /** Represents a signed 32 bit int */
    SInt32Type,
    /** Represents an unsigned 64 bit int */
    UInt64Type,
    /** Represents a signed 64 bit int */
    SInt64Type,
    /** Represents a (C++) string value */
    StringType,
    /** Represents a shared pointer to a {@link JsonValue} object */
    JsonType,
    /**
     * A type modifier to represent vector types.
     * 
     * Add this value to the base enum to get a vector of that type. For 
     * eample, a vector of floats is (ArrayType+FloatType). You should use
     * BooleanTrue to represent a vector of bool.
     */
    ArrayType = 127,
    /** Represents a read into the data string at an invalid position */
    InvalidType = 255
};

#pragma mark -
#pragma mark NetworkSerializer
/**
 * A class to serialize complex data into a byte array.
 * 
 * The class {@link NetworkConnection} is only capable of transmitting byte arrays.
 * You should use this class to construct a byte array for a single message so that
 * you can transmit it.
 * 
 * This class is capable of serializing the following data types:
 *  - Floats
 *  - Doubles
 *  - 32 Bit Signed + Unsigned Integers
 *  - 64 Bit Signed + Unsigned Integers
 *  - Strings (see note below)
 *  - JsonValue (the cugl JSON class)
 *  - Vectors of all above types
 *
 * You should deserialize all of these with the {@link NetworkDeserializer}.
 * 
 * Note that if a char* (not a C++ string) is written, it will be deserialized as a 
 * std::string. The same applies to vectors of char*.
 */
class NetworkSerializer {
private:
    /** Buffer of data that has not been written out yet. */
    std::vector<uint8_t> _data;

public:
    /**
     * Creates a new Network Serializer on the stack.
     *
     * Network serializers do not have any nontrivial state and so it is unnecessary
     * to use an init method. However, we do include a static {@link #alloc} method
     * for creating shared pointers.
     */
    NetworkSerializer() {} 

    /**
     * Returns a newly created Network Serializer.
     *
     * This method is solely include for convenience purposes.
     *
     * @return a newly created Network Serializer.
     */
    static std::shared_ptr<NetworkSerializer> alloc() {
        return std::make_shared<NetworkSerializer>();
    }
    
    /**
     * Writes a single boolean value.
     *
     * Values will be deserialized on other machines in the same order they were written 
     * in. Pass the result of {@link #serialize} to the {@link NetworkConnection} to send 
     * all values buffered up to this point.
     *
     * @param b The value to write
     */
    void writeBool(bool b);

    /**
     * Writes a single float value.
     *
     * Values will be deserialized on other machines in the same order they were written 
     * in. Pass the result of {@link #serialize} to the {@link NetworkConnection} to send 
     * all values buffered up to this point.
     *
     * @param f The value to write
     */
    void writeFloat(float f);

    /**
     * Writes a single double value.
     *
     * Values will be deserialized on other machines in the same order they were written 
     * in. Pass the result of {@link #serialize} to the {@link NetworkConnection} to send 
     * all values buffered up to this point.
     *
     * @param d The value to write
     */
    void writeDouble(double d);

    /**
     * Writes a single unsigned (32 bit) int value.
     *
     * Values will be deserialized on other machines in the same order they were written 
     * in. Pass the result of {@link #serialize} to the {@link NetworkConnection} to send 
     * all values buffered up to this point.
     *
     * @param i The value to write
     */
    void writeUint32(Uint32 i);

    /**
     * Writes a single unsigned (64 bit) int value.
     *
     * Values will be deserialized on other machines in the same order they were written 
     * in. Pass the result of {@link #serialize} to the {@link NetworkConnection} to send 
     * all values buffered up to this point.
     *
     * @param i The value to write
     */
    void writeUint64(Uint64 i);

    /**
     * Writes a single signed (32 bit) int value.
     *
     * Values will be deserialized on other machines in the same order they were written 
     * in. Pass the result of {@link #serialize} to the {@link NetworkConnection} to send 
     * all values buffered up to this point.
     *
     * @param i The value to write
     */
    void writeSint32(Sint32 i);

    /**
     * Writes a single signed (64 bit) int value.
     *
     * Values will be deserialized on other machines in the same order they were written 
     * in. Pass the result of {@link #serialize} to the {@link NetworkConnection} to send 
     * all values buffered up to this point.
     *
     * @param i The value to write
     */
    void writeSint64(Sint64 i);
    
    /**
     * Writes a single string value.
     *
     * Values will be deserialized on other machines in the same order they were written 
     * in. Pass the result of {@link #serialize} to the {@link NetworkConnection} to send 
     * all values buffered up to this point.
     *
     * @param s The value to write
     */
    void writeString(std::string s);

    /**
     * Writes a single string value.
     *
     * Values will be deserialized on other machines in the same order they were written 
     * in. Pass the result of {@link #serialize} to the {@link NetworkConnection} to send 
     * all values buffered up to this point.
     *
     * Note that this value will be deserialized by {@link NetworkSerializer} as a
     * `std::string` object.
     *
     * @param s The value to write
     */
    void writeChars(char* s);

    /**
     * Writes a single {@link JsonValue}.
     *
     * Values will be deserialized on other machines in the same order they were written 
     * in. Pass the result of {@link #serialize} to the {@link NetworkConnection} to send 
     * all values buffered up to this point.
     *
     * @param j The value to write
     */
    void writeJson(const std::shared_ptr<JsonValue>& j);

    /**
     * Writes a vector of boolean values.
     *
     * Values will be deserialized on other machines in the same order they were written 
     * in. Pass the result of {@link #serialize} to the {@link NetworkConnection} to send 
     * all values buffered up to this point.
     * 
     * @param v The vector to write
     */
    void writeBoolVector(std::vector<bool> b);

    /**
     * Writes a vector of float values.
     *
     * Values will be deserialized on other machines in the same order they were written 
     * in. Pass the result of {@link #serialize} to the {@link NetworkConnection} to send 
     * all values buffered up to this point.
     * 
     * @param v The vector to write
     */
    void writeFloatVector(std::vector<float> f);

    /**
     * Writes a vector of double values.
     *
     * Values will be deserialized on other machines in the same order they were written 
     * in. Pass the result of {@link #serialize} to the {@link NetworkConnection} to send 
     * all values buffered up to this point.
     * 
     * @param v The vector to write
     */
    void writeDoubleVector(std::vector<double> d);

    /**
     * Writes a vector of unsigned (32 bit) int values.
     *
     * Values will be deserialized on other machines in the same order they were written 
     * in. Pass the result of {@link #serialize} to the {@link NetworkConnection} to send 
     * all values buffered up to this point.
     * 
     * @param v The vector to write
     */
    void writeUint32Vector(std::vector<Uint32> i);

    /**
     * Writes a vector of unsigned (64 bit) int values.
     *
     * Values will be deserialized on other machines in the same order they were written 
     * in. Pass the result of {@link #serialize} to the {@link NetworkConnection} to send 
     * all values buffered up to this point.
     * 
     * @param v The vector to write
     */
    void writeUint64Vector(std::vector<Uint64> i);

    /**
     * Writes a vector of signed (32 bit) int values.
     *
     * Values will be deserialized on other machines in the same order they were written 
     * in. Pass the result of {@link #serialize} to the {@link NetworkConnection} to send 
     * all values buffered up to this point.
     * 
     * @param v The vector to write
     */
    void writeSint32Vector(std::vector<Sint32> i);

    /**
     * Writes a vector of signed (64 bit) int values.
     *
     * Values will be deserialized on other machines in the same order they were written 
     * in. Pass the result of {@link #serialize} to the {@link NetworkConnection} to send 
     * all values buffered up to this point.
     * 
     * @param v The vector to write
     */
    void writeSint64Vector(std::vector<Sint64> i);
    
    /**
     * Writes a vector of string values.
     *
     * Values will be deserialized on other machines in the same order they were written 
     * in. Pass the result of {@link #serialize} to the {@link NetworkConnection} to send 
     * all values buffered up to this point.
     * 
     * @param v The vector to write
     */
    void writeStringVector(std::vector<std::string> s);

    /**
     * Writes a vector of string values.
     *
     * Values will be deserialized on other machines in the same order they were written 
     * in. Pass the result of {@link #serialize} to the {@link NetworkConnection} to send 
     * all values buffered up to this point.
     *
     * Note that the vector contents will be deserialized by {@link NetworkSerializer} as
     * `std::string` objects.
     * 
     * @param v The vector to write
     */
    void writeCharsVector(std::vector<char*> s);

    /**
     * Write a vector of {@link JsonValue} objects.
     *
     * Values will be deserialized on other machines in the same order they were written 
     * in. Pass the result of {@link #serialize} to the {@link NetworkConnection} to send 
     * all values buffered up to this point.
     * 
     * @param v The vector to write
     */
    void writeJsonVector(std::vector<std::shared_ptr<JsonValue>> j);

    /**
     * Returns a byte vector of all written values suitable for network transit.
     *
     * This method should be called after the appropriate write methods have been
     * called. This provides a vector for network transit. It will need to be 
     * deserialized on the other end by {@link NetworkDeserializer}.
     * 
     * You MUST call reset() after this method to clear the input buffer. Otherwise,
     * the next call to this method will still contain all the contents written in 
     * this call.
     * 
     * The contents of the returned vector should be treated as opaque. You should 
     * only read the output via the use of {@link NetworkDeserializer}.
     * 
     * @returns a byte vector of all written values suitable for network transit.
     */
    const std::vector<uint8_t>& serialize();

    /**
     * Clears the input buffer.
     */
    void reset();
    
};


#pragma mark -
#pragma mark NetworkDeserializer
/**
 * A class to deserializes byte arrays back into the original complex data.
 *
 * This class only handles messages serialized using {@link NetworkSerializer}.
 * You should use {@link NetworkType} to guide your deserialization process. 
 */
class NetworkDeserializer {
private:
    /** Currently loaded data */
    std::vector<uint8_t> _data;
    /** Position in the data of next byte to read */
    size_t _pos;

public:
    /**
     * Variant of possible messages to receive.
     * 
     * To understand how to use variants, see this tutorial:
     *
     * https://riptutorial.com/cplusplus/example/18604/basic-std--variant-use
     *
     * This type is to be used with the {@link #read()} method. The variant monostate 
     * represents no more content.
     */
    typedef std::variant<
        std::monostate,
        bool,
        float,
        double,
        Uint32,
        Uint64,
        Sint32,
        Sint64,
        std::string,
        std::shared_ptr<JsonValue>,
        std::vector<bool>,
        std::vector<float>,
        std::vector<double>,
        std::vector<Uint32>,
        std::vector<Uint64>,
        std::vector<Sint32>,
        std::vector<Sint64>,
        std::vector<std::string>,
        std::vector<std::shared_ptr<JsonValue>>
    > Message;

    /**
     * Creates a new Network Deserializer on the stack.
     *
     * Network deserializers do not have any nontrivial state and so it is unnecessary
     * to use an init method. However, we do include a static {@link #alloc} method
     * for creating shared pointers.
     */
    NetworkDeserializer() : _pos(0) {}

    /**
     * Returns a newly created Network Deserializer.
     *
     * This method is solely include for convenience purposes.
     *
     * @return a newly created Network Deserializer.
     */
    static std::shared_ptr<NetworkDeserializer> alloc() {
        return std::make_shared<NetworkDeserializer>();
    }
    
    /**
     * Loads a new message to be read.
     * 
     * Calling this method will discard any previously loaded messages. The message must 
     * be serialized by {@link NetworkSerializer}. Otherwise, the results are unspecified.
     * 
     * Once loaded, call {@link #read} to get a single value (or vector of values). To 
     * read all of the data transmitted, call {@link #read} until it returns the 
     * monostate. The values are guaranteed to be delievered in the same order they were 
     * written.
     * 
     * If you are uncomfortable with variants, there are type-specific methods for reading
     * data. However, you will need to call {@link #nextType} to determine the correct
     * method to call.
     * 
     * @param msg The byte vector serialized by {@link NetworkSerializer}
     */
    void receive(const std::vector<uint8_t>& msg);

    /**
     * Reads the next unreturned value or vector from the currently loaded byte vector.
     * 
     * A byte vector should be loaded with the {@link #receive} method. If nothing is
     * loaded, this will return the monostate. This method also advances the read position.
     * If the end of the vector is reached, this returns the monostate.
     * 
     * The return type is a variant. You can pattern match on the variant to handle
     * different types. However, if you know what order the values were written in
     * (which you really should), you can use std::get<T>(...) to just assert the next
     * value should be of a certain type T and to extract that value directly. This 
     * avoids the overhead of a pattern match on every value. In addition, it is
     * guaranteed to never corrupt the stream (unlike the other read methods)
     */
    Message read();
    
    /**
     * Returns true if there is any data left to be read
     *
     * @return true if there is any data left to be read
     */
    bool available() const { return _pos < _data.size(); }
    
    /**
     * Returns the type of the next data value to be read.
     *
     * This method returns {@link NetworkType#InvalidType} if the stream is exhausted
     * (nothing left to be read) or corrupted.
     *
     * @return the type of the next data value to be read.
     */
     NetworkType nextType() const;
    
    /**
     * Returns a single boolean value.
     *
     * This method is only defined if {@link #nextType} has returned either BooleanTrue
     * or BooleanFalse. Otherwise, calling this method will potentially corrupt the 
     * stream.
     *
     * The method advances the read position. If called when no more data is available,
     * this method will return false.
     *
     * @return a single boolean value.
     */
    bool readBool();

    /**
     * Returns a single float value.
     *
     * This method is only defined if {@link #nextType} has returned FloatType.
     * Otherwise, calling this method will potentially corrupt the stream.
     *
     * The method advances the read position. If called when no more data is available,
     * this method will return 0.
     *
     * @return a single float value.
     */
    float readFloat();

    /**
     * Returns a single double value.
     *
     * This method is only defined if {@link #nextType} has returned DoubleType.
     * Otherwise, calling this method will potentially corrupt the stream.
     *
     * The method advances the read position. If called when no more data is available,
     * this method will return 0.
     *
     * @return a single double value.
     */
    double readDouble();

    /**
     * Returns a single unsigned (32 bit) int value.
     *
     * This method is only defined if {@link #nextType} has returned UInt32Type.
     * Otherwise, calling this method will potentially corrupt the stream.
     *
     * The method advances the read position. If called when no more data is available,
     * this method will return 0.
     *
     * @return a single unsigned (32 bit) int value.
     */
    Uint32 readUint32();

    /**
     * Returns a single signed (32 bit) int value.
     *
     * This method is only defined if {@link #nextType} has returned SInt32Type.
     * Otherwise, calling this method will potentially corrupt the stream.
     *
     * The method advances the read position. If called when no more data is available,
     * this method will return 0.
     *
     * @return a single signed (32 bit) int value.
     */
    Sint32 readSint32();

    /**
     * Returns a single unsigned (64 bit) int value.
     *
     * This method is only defined if {@link #nextType} has returned UInt64Type.
     * Otherwise, calling this method will potentially corrupt the stream.
     *
     * The method advances the read position. If called when no more data is available,
     * this method will return 0.
     *
     * @return a single unsigned (64 bit) int value.
     */
    Uint64 readUint64();
    
    /**
     * Returns a single signed (64 bit) int value.
     *
     * This method is only defined if {@link #nextType} has returned SInt64Type.
     * Otherwise, calling this method will potentially corrupt the stream.
     *
     * The method advances the read position. If called when no more data is available,
     * this method will return 0.
     *
     * @return a single signed (64 bit) int value.
     */
    Sint64 readSint64();

    /**
     * Returns a single string.
     *
     * This method is only defined if {@link #nextType} has returned StringType.
     * Otherwise, calling this method will potentially corrupt the stream.
     *
     * The method advances the read position. If called when no more data is available,
     * this method will return the empty string.
     *
     * @return a single string.
     */
    std::string readString();
    
    /**
     * Returns a single {@link JsonValue} object.
     *
     * This method is only defined if {@link #nextType} has returned JsonType.
     * Otherwise, calling this method will potentially corrupt the stream.
     *
     * The method advances the read position. If called when no more data is available,
     * this method will return nullptr.
     *
     * @return a single {@link JsonValue} object.
     */
    std::shared_ptr<JsonValue> readJson();

    /**
     * Returns a vector of boolean values.
     *
     * This method is only defined if {@link #nextType} has returned (ArrayType+BooleanTrue). 
     * Otherwise, calling this method will potentially corrupt the stream.
     *
     * The method advances the read position. If called when no more data is available,
     * this method will return false.
     *
     * @return a vector of boolean values.
     */
    std::vector<bool> readBoolVector();

    /**
     * Returns a vector of float values.
     *
     * This method is only defined if {@link #nextType} has returned (ArrayType+FloatType).
     * Otherwise, calling this method will potentially corrupt the stream.
     *
     * The method advances the read position. If called when no more data is available,
     * this method will return 0.
     *
     * @return a vector of float values.
     */
    std::vector<float> readFloatVector();

    /**
     * Returns a vector of double values.
     *
     * This method is only defined if {@link #nextType} has returned (ArrayType+DoubleType).
     * Otherwise, calling this method will potentially corrupt the stream.
     *
     * The method advances the read position. If called when no more data is available,
     * this method will return 0.
     *
     * @return a vector of double values.
     */
    std::vector<double> readDoubleVector();

    /**
     * Returns a vector of unsigned (32 bit) int values.
     *
     * This method is only defined if {@link #nextType} has returned (ArrayType+UInt32Type).
     * Otherwise, calling this method will potentially corrupt the stream.
     *
     * The method advances the read position. If called when no more data is available,
     * this method will return 0.
     *
     * @return a vector of unsigned (32 bit) int values.
     */
    std::vector<Uint32> readUint32Vector();

    /**
     * Returns a vector of signed (32 bit) int values.
     *
     * This method is only defined if {@link #nextType} has returned (ArrayType+SInt32Type).
     * Otherwise, calling this method will potentially corrupt the stream.
     *
     * The method advances the read position. If called when no more data is available,
     * this method will return 0.
     *
     * @return a vector of signed (32 bit) int values.
     */
    std::vector<Sint32> readSint32Vector();

    /**
     * Returns a vector of unsigned (64 bit) int values.
     *
     * This method is only defined if {@link #nextType} has returned (ArrayType+UInt64Type).
     * Otherwise, calling this method will potentially corrupt the stream.
     *
     * The method advances the read position. If called when no more data is available,
     * this method will return 0.
     *
     * @return a vector of unsigned (64 bit) int values.
     */
    std::vector<Uint64> readUint64Vector();
    
    /**
     * Returns a vector of signed (64 bit) int values.
     *
     * This method is only defined if {@link #nextType} has returned (ArrayType+SInt32Type).
     * Otherwise, calling this method will potentially corrupt the stream.
     *
     * The method advances the read position. If called when no more data is available,
     * this method will return 0.
     *
     * @return a vector of signed (64 bit) int values.
     */
    std::vector<Sint64> readSint64Vector();

    /**
     * Returns a vector of strings.
     *
     * This method is only defined if {@link #nextType} has returned (ArrayType+StringType).
     * Otherwise, calling this method will potentially corrupt the stream.
     *
     * The method advances the read position. If called when no more data is available,
     * this method will return the empty string.
     *
     * @return a vector of strings.
     */
    std::vector<std::string> readStringVector();
    
    /**
     * Returns a vector of {@link JsonValue} objects.
     *
     * This method is only defined if {@link #nextType} has returned (ArrayType+JsonType).
     * Otherwise, calling this method will potentially corrupt the stream.
     *
     * The method advances the read position. If called when no more data is available,
     * this method will return nullptr.
     *
     * @return a vector of {@link JsonValue} objects.
     */
    std::vector<std::shared_ptr<JsonValue>> readJsonVector();

    /**
     * Clears the buffer and ignore any remaining data in it.
     */
    void reset();
};

}

#endif // __CU_NETWORK_SERIALIZER_H__
