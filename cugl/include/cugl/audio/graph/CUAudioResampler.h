//
//  CUAudioResampler.h
//  Cornell University Game Library (CUGL)
//
//  This module provides a graph node for converting from one sample rate to
//  another. This is is necessary for cross-platform reasons as iPhones are
//  very stubborn about delivering any requested sampling rates other than 48000.
//
//  This module uses a custom resampling algorithm because SDL_AudioStream is
//  (unfortunately) broken. The resampling algorithm in SDL_AudioStream is perfect
//  (and indeed our code is a slight optimization of this algorithm). However,
//  the page buffering is buggy and can fail, causing audio to cut out. This new
//  paging scheme causes some minor round-off issues (compared to the original
//  approach), but the filter removes any alias effects that may be caused from
//  this error.
//
//  CUGL MIT License:
//
//     This software is provided 'as-is', without any express or implied
//     warranty.  In no event will the authors be held liable for any damages
//     arising from the use of this software.
//
//     Permission is granted to anyone to use this software for any purpose,
//     including commercial applications, and to alter it and redistribute it
//     freely, subject to the following restrictions:
//
//  1. The origin of this software must not be misrepresented; you must not
//     claim that you wrote the original software. If you use this software
//     in a product, an acknowledgment in the product documentation would be
//     appreciated but is not required.
//
//  2. Altered source versions must be plainly marked as such, and must not
//     be misrepresented as being the original software.
//
//  3. This notice may not be removed or altered from any source distribution.
//
//  Author: Walker White
//  Version: 6/5/21
//
#ifndef __CU_AUDIO_RESAMPLER_H__
#define __CU_AUDIO_RESAMPLER_H__
#include <cugl/audio/graph/CUAudioNode.h>
#include <SDL/SDL.h>
#include <mutex>
#include <atomic>

namespace cugl {
    
    /**
     * The audio graph classes.
     *
     * This internal namespace is for the audio graph clases.  It was chosen
     * to distinguish this graph from other graph class collections, such as the
     * scene graph collections in {@link scene2}.
     */
    namespace audio {
/**
 * This class provides a graph node for converting from one sample rate to another.
 *
 * The node uses a kaiser-windowed sinc filter to perform continuous resampling on
 * a potentially infinite audio stream.  This is is necessary for cross-platform
 * reasons as iPhones are very stubborn about delivering any requested sampling
 * rates other than 48000.
 *
 * The filter is configurable.  You can set the number of zero crossings, as well
 * as the attentionuation factor in decibels. Details behind the filter design of
 * this resampler can be found here
 *
 *     https://tomroelandts.com/articles/how-to-create-a-configurable-filter-using-a-kaiser-window
 *
 * This is a dynamic resampler.  While the output sampling rate is fixed, the
 * input is not.  It will readjust the conversion filter to match the sampling
 * rate of the input node whenever the input node changes.
 *
 * The audio graph should only be accessed in the main thread.  In addition,
 * no methods marked as AUDIO THREAD ONLY should ever be accessed by the
 * user.
 *
 * This class does not support any actions for the {@link AudioNode#setCallback}.
 */
class AudioResampler : public AudioNode {
private:
    /** Mutex to protect more sophisticated synchronization */
    mutable std::mutex _buffmtex;

    /** The input node to resample from */
    std::shared_ptr<AudioNode> _input;
    /** The currently supported input sample rate */
    std::atomic<Uint32> _inputrate;

    /** The number of zero crossings */
    std::atomic<Uint32> _zero_cross;
    /** The sample bit precision */
    std::atomic<Uint32> _bit_precision;
    /** Filter attenuation in decibels */
    std::atomic<float> _stopband;
    /** The number of samples per zero crossing */
    Uint32 _per_crossing;

    /** The filter (table) size */
    size_t _filter_size;
    /** The filter coefficients */
    float* _filter_table;
    /** The filter coefficient differences */
    float* _filter_diffs;
    
    /** Intermediate read buffer */
    /** The intermediate sampling buffer */
    float* _cvtbuffer;
    /** The capacity of the sampling buffer */
    Uint32 _capacity;
    /** The supported page size for filtering */
    Uint32 _pagesize;
    /** The amount of data currently available in the sampling buffer */
    Uint32 _cvtavail;
    /** The offset for the next (unconsumed) bit of data in the buffer */
    double _cvtoffset;

public:
#pragma mark -
#pragma mark Constructors
    /**
     * Creates a degenerate audio resampler.
     *
     * The node has not been initialized, so it is not active.  The node
     * must be initialized to be used.
     */
    AudioResampler();
    
    /**
     * Deletes the audio resampler, disposing of all resources
     */
    ~AudioResampler() { dispose(); }
    
    /**
     * Initializes a resampler with 2 channels at 48000 Hz.
     *
     * This sample rate of the output of this node is 48000 Hz, but the input
     * sample rate depends on the input node, which can change over time. However,
     * the input node must agree with number of channels, which is fixed.
     *
     * @return true if initialization was successful
     */
    virtual bool init() override;
    
    /**
     * Initializes a resampler with the given channels and sample rate.
     *
     * This sample rate is the output rate of this node.  The input same rate
     * depends on the input node, which can change over time. However, the
     * input node must agree with number of channels, which is fixed.
     *
     * @param channels  The number of audio channels
     * @param rate      The output sample rate (frequency) in Hz
     *
     * @return true if initialization was successful
     */
    virtual bool init(Uint8 channels, Uint32 rate) override;
    
    /**
     * Initializes a resampler with the given input node and sample rate.
     *
     * This node acquires the channels of the input, but will use the given
     * sample rate as its output rate. If input is nullptr, this method will
     * fail.
     *
     * @param input     The audio node to resample
     * @param rate      The output sample rate (frequency) in Hz
     *
     * @return true if initialization was successful
     */
    bool init(const std::shared_ptr<AudioNode>& input, Uint32 rate);
    
    /**
     * Disposes any resources allocated for this resampler.
     *
     * The state of the node is reset to that of an uninitialized constructor.
     * Unlike the destructor, this method allows the node to be reinitialized.
     */
    virtual void dispose() override;
    

#pragma mark -
#pragma mark Static Constructors
    /**
     * Returns a newly allocated resampler with 2 channels at 48000 Hz.
     *
     * This sample rate of the output of this node is 48000 Hz, but the input
     * sample rate depends on the input node, which can change over time. However,
     * the input node must agree with number of channels, which is fixed.
     *
     * @return a newly allocated resampler with 2 channels at 48000 Hz.
     */
    static std::shared_ptr<AudioResampler> alloc() {
        std::shared_ptr<AudioResampler> result = std::make_shared<AudioResampler>();
        return (result->init() ? result : nullptr);
    }
    
    /**
     * Returns a newly allocated resampler with the given channels and sample rate.
     *
     * This sample rate is the output rate of this node.  The input same rate
     * depends on the input node, which can change over time. However, the
     * input node must agree with number of channels, which is fixed.
     *
     * @param channels  The number of audio channels
     * @param rate      The output sample rate (frequency) in HZ
     *
     * @return a newly allocated resampler with the given channels and sample rate.
     */
    static std::shared_ptr<AudioResampler> alloc(Uint8 channels, Uint32 rate) {
        std::shared_ptr<AudioResampler> result = std::make_shared<AudioResampler>();
        return (result->init(channels,rate) ? result : nullptr);
    }
    
    /**
     * Returns a newly allocated resampler with the given input node and sample rate.
     *
     * This node acquires the channels of the input, but will use the given
     * sample rate as its output rate. If input is nullptr, this method will
     * fail.
     *
     * @param input     The audio node to resample
     * @param rate      The output sample rate (frequency) in Hz
     *
     * @return a newly allocated resampler with the given input node and sample rate.
     */
    static std::shared_ptr<AudioResampler> alloc(const std::shared_ptr<AudioNode>& input, Uint32 rate) {
        std::shared_ptr<AudioResampler> result = std::make_shared<AudioResampler>();
        return (result->init(input,rate) ? result : nullptr);
    }
    
#pragma mark -
#pragma mark Audio Graph
    /**
     * Attaches an audio node to this resampler.
     *
     * This method will reset the resampler stream if the input has a different
     * rate than the previous input value (and is not the same rate as the
     * output).  It will fail if the input does not have the same number of
     * channels as this resampler.
     *
     * @param node  The audio node to resample
     *
     * @return true if the attachment was successful
     */
    bool attach(const std::shared_ptr<AudioNode>& node);
    
    /**
     * Detaches an audio node from this resampler.
     *
     * If the method succeeds, it returns the audio node that was removed.
     * This method will not automatically reset the sampling stream.
     *
     * @return  The audio node to detach (or null if failed)
     */
    std::shared_ptr<AudioNode> detach();
    
    /**
     * Returns the input node of this resampler.
     *
     * @return the input node of this resampler.
     */
    std::shared_ptr<AudioNode> getInput() const { return _input; }

#pragma mark -
#pragma mark Filter Properties
    /**
     * Returns the input sample rate of this filter.
     *
     * This value is distinct from {@link AudioNode#getRate()}, which is the *output*
     * sample rate of this node. Instead, this value is the sample rate of any audio
     * node connected to this one via the {@link #attach} method.
     *
     * Normally this value is assigned when a new audio node is attached. However,
     * changing this value requires that the underlying read buffer be resized. Hence,
     * by setting this value ahead of time (and making sure that all attached input
     * nodes match this sample rate), you can improve the performance of this filter.
     *
     * Assigning this value while there is still an attached audio node has undefined
     * behavior.
     *
     * @return the input sample rate of this filter.
     */
    Uint32 getInputRate() const { return _inputrate.load(std::memory_order_relaxed); };
    
    /**
     * Sets the input sample rate of this filter.
     *
     * This value is distinct from {@link AudioNode#getRate()}, which is the *output*
     * sample rate of this node. Instead, this value is the sample rate of any audio
     * node connected to this one via the {@link #attach} method.
     *
     * Normally this value is assigned when a new audio node is attached. However,
     * changing this value requires that the underlying read buffer be resized. Hence,
     * by setting this value ahead of time (and making sure that all attached input
     * nodes match this sample rate), you can improve the performance of this filter.
     *
     * Assigning this value while there is still an attached audio node has undefined
     * behavior.
     *
     * @param value The input sample rate of this filter.
     */
    void setInputRate(Uint32 value);
    
    /**
     * Returns the stopband attentuation for this filter in dB
     *
     * This value is described in more detail here:
     *
     *     https://tomroelandts.com/articles/how-to-create-a-configurable-filter-using-a-kaiser-window
     *
     * By default, this value is 80.0.
     *
     * @return the ripple factor for this filter in dB
     */
    float getStopband() const { return _stopband.load(std::memory_order_relaxed); };
    
    /**
     * Sets the stopband attentuation for this filter in dB
     *
     * This value is described in more detail here:
     *
     *     https://tomroelandts.com/articles/how-to-create-a-configurable-filter-using-a-kaiser-window
     *
     * By default, this value is 80.0.
     *
     * @param value The ripple factor for this filter in dB
     */
    void setStopband(float value);

    /**
     * Returns the bit precision for audio sent to this filter.
     *
     * Even though CUGL processes all audio data as floats, that does not mean that the
     * audio on this platform is guaranteed to have 32 bit precision.  Indeed, on Android,
     * most audio is processed at 16 bit precision, and many audio files are recorded at
     * this level of precision as well. Hence this filter assumes 16 bit precision by default.
     *
     * This is relevant for the size of the filter to process the audio. Each additional bit
     * doubles the size of the filter table used for the convolution.  A 16 bit filter uses
     * a very reasonable 512 entries per zero crossing. On the other hand, a 32 bit filter
     * would require 131072 entries per zero crossing. Given the limitations of real-time
     * resampling, it typically does not make much sense to assume more than 16 bits.
     *
     * @return the bit precision for audio sent to this filter.
     */
    Uint32 getBitPrecision() const { return _bit_precision.load(std::memory_order_relaxed); };
    
    /**
     * Sets the bit precision for audio sent to this filter.
     *
     * Even though CUGL processes all audio data as floats, that does not mean that the
     * audio on this platform is guaranteed to have 32 bit precision.  Indeed, on Android,
     * most audio is processed at 16 bit precision, and many audio files are recorded at
     * this level of precision as well. Hence this filter assumes 16 bit precision by default.
     *
     * This is relevant for the size of the filter to process the audio. Each additional bit
     * doubles the size of the filter table used for the convolution.  A 16 bit filter uses
     * a very reasonable 512 entries per zero crossing. On the other hand, a 32 bit filter
     * would require 131072 entries per zero crossing. Given the limitations of real-time
     * resampling, it typically does not make much sense to assume more than 16 bits.
     *
     * @param value The bit precision for audio sent to this filter.
     */
    void setBitPrecision(Uint32 value);

    /**
     * Returns the number of zero-crossings of this filter.
     *
     * The zero-crossings of a sinc filter are relevant because the determine the number
     * of coefficients in a single filter convolution. For X zero-crossings, a single
     * output sample requires 2*(X-1) input computations. Increasing this value can give
     * some increased value in filter. However, the droppoff for sinc filters is large
     * enough that eventually that large enough values will have no discernable effect.
     *
     * The default number of zero crossing is 5, meaning that this filter roughly causes
     * an 8x-10x decrease in performance when processing audio (when taking all the
     * relevant overhead into account).  This value is that one recommended by this
     * tutorial website:
     *
     *     https://www.dsprelated.com/freebooks/pasp/Windowed_Sinc_Interpolation.html
     *
     * @return the number of zero-crossings of this filter.
     */
    Uint32 getZeroCrossings() const { return _zero_cross.load(std::memory_order_relaxed); };
    
    /**
     * Sets the number of zero-crossings of this filter.
     *
     * The zero-crossings of a sinc filter are relevant because the determine the number
     * of coefficients in a single filter convolution. For X zero-crossings, a single
     * output sample requires 2*(X-1) input computations. Increasing this value can give
     * some increased value in filter. However, the droppoff for sinc filters is large
     * enough that eventually that large enough values will have no discernable effect.
     *
     * The default number of zero crossing is 5, meaning that this filter roughly causes
     * an 8x-10x decrease in performance when processing audio (when taking all the
     * relevant overhead into account).  This value is that one recommended by this
     * tutorial website:
     *
     *     https://www.dsprelated.com/freebooks/pasp/Windowed_Sinc_Interpolation.html
     *
     * @param value The number of zero-crossings of this filter.
     */
    void setZeroCrossings(Uint32 value);
    

#pragma mark -
#pragma mark Playback Control
    /**
     * Returns true if this resampler has no more data.
     *
     * An audio node is typically completed if it return 0 (no frames read) on
     * subsequent calls to {@link read()}.  However, for infinite-running
     * audio threads, it is possible for this method to return true even when
     * data can still be read; in that case the node is notifying that it
     * should be shut down.
     *
     * @return true if this audio node has no more data.
     */
    virtual bool completed() override;
    
    /**
     * Reads up to the specified number of frames into the given buffer
     *
     * AUDIO THREAD ONLY: Users should never access this method directly.
     * The only exception is when the user needs to create a custom subclass
     * of this AudioOutput.
     *
     * The buffer should have enough room to store frames * channels elements.
     * The channels are interleaved into the output buffer.
     *
     * This method will always forward the read position.
     *
     * @param buffer    The read buffer to store the results
     * @param frames    The maximum number of frames to read
     *
     * @return the actual number of frames read
     */
    virtual Uint32 read(float* buffer, Uint32 frames) override;
    
#pragma mark -
#pragma mark Optional Methods
    /**
     * Marks the current read position in the audio steam.
     *
     * DELEGATED METHOD: This method delegates its call to the input node.  It
     * returns false if there is no input node or if this method is unsupported
     * in that node
     *
     * This method is typically used by {@link reset()} to determine where to
     * restore the read position. For some nodes (like {@link AudioInput}),
     * this method may start recording data to a buffer, which will continue
     * until {@link reset()} is called.
     *
     * It is possible for {@link reset()} to be supported even if this method
     * is not.
     *
     * @return true if the read position was marked.
     */
    virtual bool mark() override;
    
    /**
     * Clears the current marked position.
     *
     * DELEGATED METHOD: This method delegates its call to the input node.  It
     * returns false if there is no input node or if this method is unsupported
     * in that node
     *
     * If the method {@link mark()} started recording to a buffer (such as
     * with {@link AudioInput}), this method will stop recording and release
     * the buffer.  When the mark is cleared, {@link reset()} may or may not
     * work depending upon the specific node.
     *
     * @return true if the read position was marked.
     */
    virtual bool unmark() override;
    
    /**
     * Resets the read position to the marked position of the audio stream.
     *
     * DELEGATED METHOD: This method delegates its call to the input node.  It
     * returns false if there is no input node or if this method is unsupported
     * in that node
     *
     * When no {@link mark()} is set, the result of this method is node
     * dependent.  Some nodes (such as {@link AudioPlayer}) will reset to the
     * beginning of the stream, while others (like {@link AudioInput}) only
     * support a rest when a mark is set. Pay attention to the return value of
     * this method to see if the call is successful.
     *
     * @return true if the read position was moved.
     */
    virtual bool reset() override;
    
    /**
     * Advances the stream by the given number of frames.
     *
     * DELEGATED METHOD: This method delegates its call to the input node.  It
     * returns -1 if there is no input node or if this method is unsupported
     * in that node
     *
     * This method only advances the read position, it does not actually
     * read data into a buffer. This method is generally not supported
     * for nodes with real-time input like {@link AudioInput}.
     *
     * @param frames    The number of frames to advace
     *
     * @return the actual number of frames advanced; -1 if not supported
     */
    virtual Sint64 advance(Uint32 frames) override;
    
    /**
     * Returns the current frame position of this audio node
     *
     * DELEGATED METHOD: This method delegates its call to the input node.  It
     * returns -1 if there is no input node or if this method is unsupported
     * in that node
     *
     * In some nodes like {@link AudioInput}, this method is only supported
     * if {@link mark()} is set.  In that case, the position will be the
     * number of frames since the mark. Other nodes like {@link AudioPlayer}
     * measure from the start of the stream.
     *
     * @return the current frame position of this audio node.
     */
    virtual Sint64 getPosition() const override;
    
    /**
     * Sets the current frame position of this audio node.
     *
     * DELEGATED METHOD: This method delegates its call to the input node.  It
     * returns -1 if there is no input node or if this method is unsupported
     * in that node
     *
     * In some nodes like {@link AudioInput}, this method is only supported
     * if {@link mark()} is set.  In that case, the position will be the
     * number of frames since the mark. Other nodes like {@link AudioPlayer}
     * measure from the start of the stream.
     *
     * @param position  the current frame position of this audio node.
     *
     * @return the new frame position of this audio node.
     */
    virtual Sint64 setPosition(Uint32 position) override;
    
    /**
     * Returns the elapsed time in seconds.
     *
     * DELEGATED METHOD: This method delegates its call to the input node.  It
     * returns -1 if there is no input node or if this method is unsupported
     * in that node
     *
     * In some nodes like {@link AudioInput}, this method is only supported
     * if {@link mark()} is set.  In that case, the times will be the
     * number of seconds since the mark. Other nodes like {@link AudioPlayer}
     * measure from the start of the stream.
     *
     * @return the elapsed time in seconds.
     */
    virtual double getElapsed() const override;
    
    /**
     * Sets the read position to the elapsed time in seconds.
     *
     * DELEGATED METHOD: This method delegates its call to the input node.  It
     * returns -1 if there is no input node or if this method is unsupported
     * in that node
     *
     * In some nodes like {@link AudioInput}, this method is only supported
     * if {@link mark()} is set.  In that case, the new time will be meaured
     * from the mark. Other nodes like {@link AudioPlayer} measure from the
     * start of the stream.
     *
     * @param time  The elapsed time in seconds.
     *
     * @return the new elapsed time in seconds.
     */
    virtual double setElapsed(double time) override;
    
    /**
     * Returns the remaining time in seconds.
     *
     * DELEGATED METHOD: This method delegates its call to the input node.  It
     * returns -1 if there is no input node or if this method is unsupported
     * in that node
     *
     * In some nodes like {@link AudioInput}, this method is only supported
     * if {@link setRemaining()} has been called.  In that case, the node will
     * be marked as completed after the given number of seconds.  This may or may
     * not actually move the read head.  For example, in {@link AudioPlayer} it
     * will skip to the end of the sample.  However, in {@link AudioInput} it
     * will simply time out after the given time.
     *
     * @return the remaining time in seconds.
     */
    virtual double getRemaining() const override;
    
    /**
     * Sets the remaining time in seconds.
     *
     * DELEGATED METHOD: This method delegates its call to the input node.  It
     * returns -1 if there is no input node or if this method is unsupported
     * in that node
     *
     * If this method is supported, then the node will be marked as completed
     * after the given number of seconds.  This may or may not actually move
     * the read head.  For example, in {@link AudioPlayer} it will skip to the
     * end of the sample.  However, in {@link AudioInput} it will simply time
     * out after the given time.
     *
     * @param time  The remaining time in seconds.
     *
     * @return the new remaining time in seconds.
     */
    virtual double setRemaining(double time) override;
    
private:
#pragma mark -
#pragma mark Filter Algorithm
    /**
     * Sets up the filter table for resampling.
     *
     * This filter table must be recomputed any time any of the filter properties
     * are altered. These properties include {@link #getStopband},
     * {@link #getBitPrecision} and {@link #getZeroCrossings}. However, the table
     * does **not** need to be recomputed when the input rate changes.
     */
    void setup();
    
    /**
     * Filters a single frame (for all channels) of output audio
     *
     * This method processes all of the channels for the current audio frame and
     * stores the results in buffer (in order by channel). The current audio frame
     * is determined by the _cvtoffset value.
     *
     * The additional parameters passed to this method are to ensure thread safety.
     * For example, inrate is the input sampling rate at the time of the buffer
     * computation, and not necessarily the current input rate.
     *
     * @param buffer    The buffer to store the audio frame
     * @param inrate    The input rate at the time of computation
     * @param limit     The number of elements in the intermediate buffer
     */
    void filter(float* buffer, double inrate, Uint32 limit);
    
};
    }
}

#endif /* __CU_AUDIO_RESAMPLER_H__ */
