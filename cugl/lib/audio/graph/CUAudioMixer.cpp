//
//  CUAudioMixer.cpp
//  Cornell University Game Library (CUGL)
//
//  Thie module provides an audio graph node for mixing together several input
//  streams into a single output stream. The input nodes must all have the same
//  same number of channels and sampling rate.
//
//  Mixing works by adding together all of the streams.  This means that the
//  results may exceed the range [-1,1], causing clipping.  The mixer provides
//  a "soft-knee" option for confining the results to the range [-1,1].
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
//  Version: 11/7/18
//
#include <cugl/audio/graph/CUAudioMixer.h>
#include <cugl/audio/CUAudioDevices.h>
#include <cugl/math/dsp/CUDSPMath.h>
#include <cugl/util/CUDebug.h>
#include <atomic>

using namespace cugl;
using namespace cugl::audio;

/** The default number of inputs supported (typically 8) */
const Uint8 AudioMixer::DEFAULT_WIDTH = 8;
/** The standard knee value for preventing clipping */
const float AudioMixer::DEFAULT_KNEE  = 0.9;


#pragma mark -
#pragma mark Constructors
/**
 * Creates a degenerate mixer that takes no inputs
 *
 * The mixer has no width and therefore cannot accept any inputs. The mixer
 * must be initialized to be used.
 */
AudioMixer::AudioMixer() :
_width(0),
_knee(-1),
_capacity(0),
_inputs(nullptr),
_buffer(nullptr) {
    _classname = "AudioScheduler";
#if CU_PLATFORM == CU_PLATFORM_ANDROID
	// Android handles clipping very badly.
	_knee = AudioMixer::DEFAULT_KNEE;
#endif
}

/**
 * Initializes the mixer with default stereo settings
 *
 * The number of channels is two, for stereo output.  The sample rate is
 * the modern standard of 48000 HZ.
 *
 * These values determine the buffer the structure for all {@link read}
 * operations.  In addition, they also detemine exactly which audio nodes
 * are supported by this mixer.  A mixer can only mix nodes that agree
 * on both sample rate and frequency.
 *
 * @param width     The number of audio nodes that may be attached to this mixer
 *
 * @return true if initialization was successful
 */
bool AudioMixer::init() {
    return init(DEFAULT_WIDTH,DEFAULT_CHANNELS,DEFAULT_SAMPLING);
}

/**
 * Initializes the mixer with default stereo settings
 *
 * The number of channels is two, for stereo output.  The sample rate is
 * the modern standard of 48000 HZ.
 *
 * These values determine the buffer the structure for all {@link read}
 * operations.  In addition, they also detemine exactly which audio nodes
 * are supported by this mixer.  A mixer can only mix nodes that agree
 * on both sample rate and frequency.
 *
 * @param width     The number of audio nodes that may be attached to this mixer
 *
 * @return true if initialization was successful
 */
bool AudioMixer::init(Uint8 width) {
    return init(width,DEFAULT_CHANNELS,DEFAULT_SAMPLING);
}

/**
 * Initializes the mixer with the given number of channels and sample rate
 *
 * These values determine the buffer the structure for all {@link read}
 * operations.  In addition, they also detemine exactly which audio nodes
 * are supported by this mixer.  A mixer can only mix nodes that agree
 * on both sample rate and frequency.
 *
 * @param width     The number of audio nodes that may be attached to this mixer
 * @param channels  The number of audio channels
 * @param rate      The sample rate (frequency) in HZ
 *
 * @return true if initialization was successful
 */
bool AudioMixer::init(Uint8 channels, Uint32 rate) {
    return init(DEFAULT_WIDTH,channels,rate);
}

/**
 * Initializes the mixer with the given number of channels and sample rate
 *
 * These values determine the buffer the structure for all {@link read}
 * operations.  In addition, they also detemine exactly which audio nodes
 * are supported by this mixer.  A mixer can only mix nodes that agree
 * on both sample rate and frequency.
 *
 * @param width     The number of audio nodes that may be attached to this mixer
 * @param channels  The number of audio channels
 * @param rate      The sample rate (frequency) in HZ
 *
 * @return true if initialization was successful
 */
bool AudioMixer::init(Uint8 width, Uint8 channels, Uint32 rate) {
    if (AudioNode::init(channels,rate)) {
        CUAssertLog(width,"Mixer width is 0");
        _width = width;
        _knee  = -1;
        _capacity = AudioDevices::get()->getReadSize();
        _inputs = new std::shared_ptr<AudioNode>[_width];
        for (int ii = 0; ii < _width; ii++) {
            _inputs[ii] = nullptr;
        }
        _buffer = (float*)malloc(_capacity*_channels*sizeof(float));
        return true;
    }
    return false;
}

/**
 * Disposes any resources allocated for this mixer
 *
 * The state of the node is reset to that of an uninitialized constructor.
 * Unlike the destructor, this method allows the node to be reinitialized.
 */
void AudioMixer::dispose() {
    if (_booted) {
        AudioNode::dispose();
        delete[] _inputs;
        free(_buffer);
        _inputs = nullptr;
        _buffer = nullptr;
        _width = 0;
        _knee  = -1;
        _capacity = 0;
    }
}


#pragma mark -
#pragma mark Audio Graph Methods
/**
 * Attaches an input node to this mixer.
 *
 * The input is attached at the given slot. Any input node previously at
 * that slot is removed (and returned by this method).
 *
 * @param slot  The slot for the input node
 * @param input The input node to attach
 *
 * @return the input node previously at the given slot
 */
std::shared_ptr<AudioNode> AudioMixer::attach(Uint8 slot, const std::shared_ptr<AudioNode>& input) {
    CUAssertLog(slot < _width, "Slot %d is out of range",slot);
    if (input == nullptr) {
        return detach(slot);
    } else if (input->getChannels() != _channels) {
        CUAssertLog(false,"AudioNode has wrong number of channels: %d vs %d",
                    input->getChannels(),_channels);
        return nullptr;
    } else if (input->getRate() != _sampling) {
        CUAssertLog(false,"AudioNode has wrong sample rate: %d vs %d",
                    input->getRate(),_sampling);
        return nullptr;
    }
    _marked.store(0,std::memory_order_relaxed);
    _offset.store(0,std::memory_order_relaxed);
    return std::atomic_exchange_explicit(_inputs+slot,input,std::memory_order_relaxed);
}

/**
 * Detaches the input node at the given slot.
 *
 * The input node detached is returned by this method.
 *
 * @param slot  The slot for the input node
 *
 * @return the input node detached from the slot
 */
std::shared_ptr<AudioNode> AudioMixer::detach(Uint8 slot) {
    CUAssertLog(slot < _width, "Slot %d is out of range",slot);
    return std::atomic_exchange_explicit(_inputs+slot,{},std::memory_order_relaxed);
}

/**
 * Returns true if this audio node has no more data.
 *
 * DELEGATED METHOD: This method delegates its call to **all** currently
 * attached input nodes.  It returns true if there are no attached input
 * nodes, or if **all** of the input nodes are complete.
 *
 * An audio node is typically completed if it return 0 (no frames read) on
 * subsequent calls to {@link read()}.
 *
 * @return true if this audio node has no more data.
 */
bool AudioMixer::completed() {
    std::lock_guard<std::mutex> lock(_mutex);
    bool success = true;
    std::shared_ptr<AudioNode> temp;
    for(int ii = 0; ii < _width; ii++) {
        temp = std::atomic_load_explicit(_inputs+ii,std::memory_order_relaxed);
        if (temp) {
            success = temp->completed() && success;
        }
    }
    return success;
}

/**
 * Reads up to the specified number of frames into the given buffer
 *
 * AUDIO THREAD ONLY: Users should never access this method directly, unless
 * part of a custom audio graph node.
 *
 * The buffer should have enough room to store frames * channels elements.
 * The channels are interleaved into the output buffer.
 *
 * Reading the buffer has no affect on the read position.  You must manually
 * move the frame position forward.  This is to allow for a frame window to
 * be reread if necessary.
 *
 * @param buffer    The read buffer to store the results
 * @param frames    The maximum number of frames to read
 *
 * @return the actual number of frames read
 */
Uint32 AudioMixer::read(float* buffer, Uint32 frames) {
    std::memset(buffer,0,frames*_channels*sizeof(float));
    frames = std::min(frames,_capacity);
    Uint32 actual = 0;
    if (!_paused.load(std::memory_order_relaxed)) {
        std::lock_guard<std::mutex> lock(_mutex);
        std::shared_ptr<AudioNode> temp;
        for(int ii = 0; ii < _width; ii++) {
            temp = std::atomic_load_explicit(_inputs+ii,std::memory_order_relaxed);
            if (temp) {
                Uint32 amt = temp->read(_buffer,frames);
                actual = std::max(amt,actual);
                if (amt < frames) {
                    std::memset(_buffer+amt,0,(frames-amt)*_channels*sizeof(float));
                }
                dsp::DSPMath::add(_buffer,buffer,buffer,frames*_channels);
            }
        }
        dsp::DSPMath::scale(buffer,_ndgain.load(std::memory_order_relaxed),buffer,frames*_channels);
        float knee = _knee.load(std::memory_order_relaxed);
        if (knee == 1) {
            dsp::DSPMath::clamp(buffer,-1,1,frames*_channels);
        } else if (knee > 0) {
            dsp::DSPMath::ease(buffer,1,knee,frames*_channels);
        }
    } else {
        std::memset(buffer,0,frames*sizeof(float)*_channels);
        actual = frames;
    }
    
    Uint64 pos = _offset.load(std::memory_order_relaxed);
    _offset.store(pos+actual,std::memory_order_relaxed);
    return actual;
}

/**
 * Sets the width of this mixer.
 *
 * The width is the number of supported input slots. This method will only
 * succeed if the mixer is paused.  Otherwise, it will fail.
 *
 * Once the width is adjusted, the children will be reassigned in order.
 * If the new width is less than the old width, children at the end of
 * the mixer will be dropped.
 *
 * @return true if the mixer width was reset
 */
bool AudioMixer::setWidth(Uint8 width) {
    if (_paused.load(std::memory_order_relaxed)) {
        std::shared_ptr<AudioNode>* replace = new std::shared_ptr<AudioNode>[width];
        Uint32 min = width < _width ? width : _width;
        for(int ii = 0; ii < width; ii++) {
            replace[ii] = ii < min ? _inputs[ii] : nullptr;
        }
        delete[] _inputs;
        _inputs = replace;
        return true;
    }
    return false;
}

#pragma mark -
#pragma mark Audio Graph Methods
/**
 * Returns the "soft knee" of this mixer, or -1 if not set
 *
 * The soft knee is used to ensure that the results fit in the range [-1,1].
 * If the knee is k, then values in the range [-k,k] are unaffected, but
 * values outside of this range are asymptotically clamped to the range
 * [-1,1], using the formula (x-k+k*k)/x.
 *
 * @return the "soft knee" of this mixer, or -1 if not set
 */
float AudioMixer::getKnee() const {
    return _knee.load(std::memory_order_relaxed);
}

/**
 * Sets the "soft knee" of this mixer.
 *
 * The soft knee is used to ensure that the results fit in the range [-1,1].
 * If the knee is k, then values in the range [-k,k] are unaffected, but
 * values outside of this range are asymptotically clamped to the range
 * [-1,1], using the formula (x-k+k*k)/x
 *
 * Setting this value to negative will disable the soft knee.  All inputs
 * will be mixed exactly with no distortion.
 *
 * @param knee  the "soft knee" of this mixer
 */
void AudioMixer::setKnee(float knee) {
    if (knee <= 0 || knee >= 1) {
        knee = -1;
    }
    _knee.store(knee,std::memory_order_relaxed);
}

#pragma mark -
#pragma mark Optional Methods
/**
 * Marks the current read position in the audio steam.
 *
 * DELEGATED METHOD: This method delegates its call to **all** currently
 * attached input nodes.  It returns false if there are no attached input
 * nodes, or if just **one** of the input nodes does not support marking.
 * However, any input that was successfully marked remains marked.
 *
 * This method is typically used by {@link #reset()} to determine where to
 * restore the read position. For some nodes (like {@link AudioInput}),
 * this method may start recording data to a buffer, which will continue
 * until {@link #reset()} is called.
 *
 * Input nodes added to the mixer after this method is called are not
 * affected.
 *
 * @return true if the read position was marked across all inputs.
 */
bool AudioMixer::mark() {
    std::lock_guard<std::mutex> lock(_mutex);
    bool success = true;
    std::shared_ptr<AudioNode> temp;
    for(int ii = 0; ii < _width; ii++) {
        temp = std::atomic_load_explicit(_inputs+ii,std::memory_order_relaxed);
        if (temp) {
            success = temp->mark() && success;
        }
    }
    _marked.store(_offset.load(std::memory_order_relaxed),std::memory_order_relaxed);
    return success;
}

/**
 * Clears the current marked position.
 *
 * DELEGATED METHOD: This method delegates its call to **all** currently
 * attached input nodes.  It returns false if there are no attached input
 * nodes, or if just **one** of the input nodes does not support marking.
 * However, any input that was successfully unmarked remains unmarked.
 *
 * If the method {@link #mark()} started recording to a buffer (such as
 * with {@link AudioInput}), this method will stop recording and release
 * the buffer.  When the mark is cleared, {@link #reset()} may or may not
 * work depending upon the specific node.
 *
 * This method is ideal for a mixer composed of {@link AudioPlayer}
 * objects. It will equally mark all of the components, keeping them in
 * sync.
 *
 * @return true if the read position was marked.
 */
bool AudioMixer::unmark() {
    std::lock_guard<std::mutex> lock(_mutex);
    bool success = true;
    std::shared_ptr<AudioNode> temp;
    for(int ii = 0; ii < _width; ii++) {
        temp = std::atomic_load_explicit(_inputs+ii,std::memory_order_relaxed);
        if (temp) {
            success = temp->unmark() && success;
        }
    }
    _marked.store(0,std::memory_order_relaxed);
    return success;
}

/**
 * Resets the read position to the marked position of the audio stream.
 *
 * DELEGATED METHOD: This method delegates its call to **all** currently
 * attached input nodes.  It returns false if there are no attached input
 * nodes, or if just **one** of the input nodes cannot be reset. However,
 * However, any input that was successfully reset remains reset.
 *
 * This method is ideal for a mixer composed of {@link AudioPlayer}
 * objects. It will equally unmark all of the components, keeping them
 * in sync.
 *
 * @return true if the read position was moved.
 */
bool AudioMixer::reset() {
    std::lock_guard<std::mutex> lock(_mutex);
    bool success = true;
    std::shared_ptr<AudioNode> temp;
    for(int ii = 0; ii < _width; ii++) {
        temp = std::atomic_load_explicit(_inputs+ii,std::memory_order_relaxed);
        if (temp) {
            success = temp->reset() && success;
        }
    }
    _offset.store(_marked.load(std::memory_order_relaxed),std::memory_order_relaxed);
    return success;
}

/**
 * Advances the stream by the given number of frames.
 *
 * DELEGATED METHOD: This method delegates its call to **all** currently
 * attached input nodes.  It returns -1 if there are no attached input
 * nodes, or if just **one** of the input nodes cannot be advanced.
 * However, any input that was successfully advanced remains advanced.
 *
 * This method is ideal for a mixer composed of {@link AudioPlayer}
 * objects. It will equally reset all of the components, keeping them in
 * sync.
 *
 * @param frames    The number of frames to advace
 *
 * @return the actual number of frames advanced; -1 if not supported
 */
Sint64 AudioMixer::advance(Uint32 frames) {
    std::lock_guard<std::mutex> lock(_mutex);
    Sint64 actual = 0;
    bool fail = false;
    std::shared_ptr<AudioNode> temp;
    for(int ii = 0; ii < _width; ii++) {
        temp = std::atomic_load_explicit(_inputs+ii,std::memory_order_relaxed);
        if (temp) {
            Sint64 amt = temp->advance(frames);
            actual = std::max(actual,amt);
            fail = fail || amt == -1;
        }
    }
    
    Uint64 pos = _offset.load(std::memory_order_relaxed);
    _offset.store(pos+actual,std::memory_order_relaxed);
    return fail ? -1 : actual;
}

/**
 * Returns the current frame position of this audio node
 *
 * If {@link #mark()} was called previously, this value is the number
 * of frames since the mark. Otherwise, it is the number of frames
 * since the start of this stream. Calling {@link #reset()} will
 * reset this position even if not all of the inputs were reset.
 *
 * This method is ideal for a mixer composed of {@link AudioPlayer}
 * objects. In that case, it will return the synchronous position
 * of all of the players.
 *
 * @return the current frame position of this audio node.
 */
Sint64 AudioMixer::getPosition() const {
    return (Sint64)_offset.load(std::memory_order_relaxed);
}

/**
 * Sets the current frame position of this audio node.
 *
 * DELEGATED METHOD: This method delegates its call to **all** currently
 * attached input nodes.  It returns -1 if there are no attached input
 * nodes, or if just **one** of the input nodes cannot be repositioned.
 * However, any input that was successfully repositioned remains
 * repositioned.
 *
 * This method is ideal for a mixer composed of {@link AudioPlayer}
 * objects. In that case, it will set the synchronous position
 * of all of the players.
 *
 * @param position  the current frame position of this audio node.
 *
 * @return the new frame position of this audio node.
 */
Sint64 AudioMixer::setPosition(Uint32 position) {
    std::lock_guard<std::mutex> lock(_mutex);
    Sint64 actual = 0;
    bool fail = false;
    std::shared_ptr<AudioNode> temp;
    for(int ii = 0; ii < _width; ii++) {
        temp = std::atomic_load_explicit(_inputs+ii,std::memory_order_relaxed);
        if (temp) {
            Sint64 amt = temp->setPosition(position);
            actual = std::max(actual,amt);
            fail = fail || amt == -1;
        }
    }
    
    _offset.store(actual,std::memory_order_relaxed);
    return fail ? -1 : actual;
}

/**
 * Returns the elapsed time in seconds.
 *
 * If {@link #mark()} was called previously, this value is the number
 * of seconds since the mark. Otherwise, it is the number of seconds
 * since the start of this stream. Calling {@link #reset()} will
 * reset this position even if not all of the inputs were reset.
 *
 * This method is ideal for a mixer composed of {@link AudioPlayer}
 * objects. In that case, it will return the synchronous elapsed
 * time of all of the players.
 *
 * @return the elapsed time in seconds.
 */
double AudioMixer::getElapsed() const {
    Uint64 offset = _offset.load(std::memory_order_relaxed);
    return (double)offset/(double)getRate();
}

/**
 * Sets the read position to the elapsed time in seconds.
 *
 * DELEGATED METHOD: This method delegates its call to **all** currently
 * attached input nodes.  It returns -1 if there are no attached input
 * nodes, or if just **one** of the input nodes cannot be repositioned.
 * However, any input that was successfully repositioned remains
 * repositioned.
 *
 * This method is ideal for a mixer composed of {@link AudioPlayer}
 * objects. In that case, it will set the synchronous position
 * of all of the players.
 *
 * @param time  The elapsed time in seconds.
 *
 * @return the new elapsed time in seconds.
 */
double AudioMixer::setElapsed(double time) {
    Uint64 position = time*getRate();
    position = setPosition((Uint32)position);
    return (double)position/(double)getRate();
}

/**
 * Returns the remaining time in seconds.
 *
 * DELEGATED METHOD: This method delegates its call to **all** currently
 * attached input nodes. The value returned is the maximum value across
 * all nodes. It returns -1 if there are no attached input node, or if
 * **any** attached node does not support this method.
 *
 * This method is ideal for a mixer composed of {@link AudioPlayer}
 * objects. In that case, it will return the maximum remaining time
 * across all of the players.
 *
 * @return the remaining time in seconds.
 */
double AudioMixer::getRemaining() const {
    // An unavoidable race condition has minor effects on accuracy
    double actual = 0;
    bool fail = false;
    std::shared_ptr<AudioNode> temp;
    for(int ii = 0; ii < _width; ii++) {
        temp = std::atomic_load_explicit(_inputs+ii,std::memory_order_relaxed);
        if (temp) {
            double amt = temp->getRemaining();
            actual = std::max(actual,amt);
            fail = fail || amt == -1;
        }
    }
    
    return fail ? -1 : actual;
}

/**
 * Sets the remaining time in seconds.
 *
 * DELEGATED METHOD: This method delegates its call to **all** currently
 * attached input nodes. The value set is the relative to the maximum
 * remaining time across all inputs. Any input with less than the remaining
 * time is advanced forward so that it remains in sync with the maximal
 * input.
 *
 * This method returns  returns -1 if there are no attached input node,
 * or if **any** attached node does not support this method. However,
 * any repositioned input will remain repositioned.
 *
 * This method is ideal for a mixer composed of {@link AudioPlayer}
 * objects. In that case, it will set the synchronous position
 * of all of the players.
 *
 * @param time  The remaining time in seconds.
 *
 * @return the new remaining time in seconds.
 */
double AudioMixer::setRemaining(double time) {
    std::lock_guard<std::mutex> lock(_mutex);
    
    // Get longest time remaining
    double actual = 0;
    bool fail = false;
    std::shared_ptr<AudioNode> temp;
    for(int ii = 0; ii < _width; ii++) {
        temp = std::atomic_load_explicit(_inputs+ii,std::memory_order_relaxed);
        if (temp) {
            double amt = temp->getRemaining();
            actual = std::max(actual,amt);
            fail = fail || amt == -1;
        }
    }
    
    Uint64 pos = _offset.load(std::memory_order_relaxed)+actual*getRate();
    
    // Now push forward
    for(int ii = 0; ii < _width; ii++) {
        temp = std::atomic_load_explicit(_inputs+ii,std::memory_order_relaxed);
        if (temp) {
            Uint64 off = temp->setPosition((Uint32)pos);
            if (off < 0) {
                double back = temp->setRemaining(time+temp->getRemaining()-actual);
                if (back < 0) {
                    fail = false;
                }
            }
        }
    }
    
    _offset.store(pos,std::memory_order_relaxed);
    return fail ? -1 : actual;
}
