"""
Makes every sound in One Shot (the .wav files in this folder)

Run it from anywhere with Python 3 and numpy:
	python OneShotGame_/Content/Sounds/make_sounds.py

Every sound is synthesized from scratch (no recordings), so there is nothing to license
and any sound can be tweaked by changing a number below and running this again.
The random numbers are seeded, so running this again without changing anything makes identical files.
To use a recorded sound instead, replace its .wav file with one that has the same name
(SoundBuilder accepts mono or stereo, 8/16/24/32-bit or 32-bit floating point, at any common sample rate).

How the sounds are made:
	* Impacts (wood, straw) are a short burst of noise for the moment of contact
		plus a few decaying sine waves at the frequencies the struck object rings at ("modes")
	* The bow is a plucked string (the Karplus-Strong algorithm: a burst of noise fed back through a delay the length of one period)
		plus a whoosh of filtered noise for the arrow leaving
	* The brass fanfare adds up harmonics whose loudness depends on how hard the note is being played,
		because brass instruments get brighter (more high harmonics) as they get louder
	* Crowd voices are a buzzing "vocal cord" source shaped by "formant" filters,
		which are the resonances of the mouth that make the difference between "ah", "ee", and "oo"
	* Applause is many people clapping: each clap is a tiny burst of filtered noise
	* A little reverb (the sound bouncing around the stadium) is added by convolving with a decaying burst of noise
"""

import math
import os
import wave

import numpy as np

SAMPLE_RATE = 44100
OUTPUT_DIRECTORY = os.path.dirname( os.path.abspath( __file__ ) )
rng = np.random.default_rng( 6320 )


# Helpers
#========

def Times( duration ):
	return np.arange( int( duration * SAMPLE_RATE ) ) / SAMPLE_RATE

def Pad( signal, duration ):
	"""Returns the signal zero-padded (or cut) to the given duration"""
	length = int( duration * SAMPLE_RATE )
	result = np.zeros( length )
	count = min( length, len( signal ) )
	result[:count] = signal[:count]
	return result

def AddAt( destination, signal, startTime ):
	start = int( startTime * SAMPLE_RATE )
	if start >= len( destination ):
		return
	count = min( len( signal ), len( destination ) - start )
	destination[start:start + count] += signal[:count]

def Noise( duration ):
	return rng.standard_normal( int( duration * SAMPLE_RATE ) )

def FilterInFrequencyDomain( signal, ResponseAtFrequency ):
	"""Filters a signal by multiplying its spectrum by a (real, smooth) frequency response"""
	length = len( signal )
	paddedLength = 1 << ( ( length - 1 ).bit_length() )
	spectrum = np.fft.rfft( signal, paddedLength )
	frequencies = np.fft.rfftfreq( paddedLength, 1.0 / SAMPLE_RATE )
	return np.fft.irfft( spectrum * ResponseAtFrequency( frequencies ), paddedLength )[:length]

def BandPassResponse( center, q ):
	# The magnitude response of a two-pole band-pass filter (1 at the center frequency)
	def Response( frequencies ):
		f = np.maximum( frequencies, 1.0 )
		return 1.0 / np.sqrt( 1.0 + ( q * ( f / center - center / f ) ) ** 2 )
	return Response

def LowPassResponse( cutoff, order = 2 ):
	return lambda frequencies: 1.0 / np.sqrt( 1.0 + ( frequencies / cutoff ) ** ( 2 * order ) )

def HighPassResponse( cutoff, order = 2 ):
	return lambda frequencies: 1.0 / np.sqrt( 1.0 + ( cutoff / np.maximum( frequencies, 1.0 ) ) ** ( 2 * order ) )

def FormantResponse( formants ):
	"""The resonances of a mouth shaping a vowel: a list of ( frequency, bandwidth, gain )"""
	def Response( frequencies ):
		total = np.zeros_like( frequencies )
		for frequency, bandwidth, gain in formants:
			total += gain * BandPassResponse( frequency, frequency / bandwidth )( frequencies )
		return total
	return Response

def DecayingSine( frequency, decayTime, duration, phase = 0.0 ):
	t = Times( duration )
	return np.sin( 2.0 * math.pi * frequency * t + phase ) * np.exp( -t / decayTime )

def Envelope( duration, points ):
	"""A piecewise-linear envelope through ( time, level ) points"""
	t = Times( duration )
	times, levels = zip( *points )
	return np.interp( t, times, levels )

def Reverb( signal, decayTime, wetness, brightness = 4000.0, seed = 0 ):
	"""Adds the reflections of a big open space: the signal convolved with a decaying burst of noise"""
	reverbRng = np.random.default_rng( seed )
	length = int( decayTime * 3.0 * SAMPLE_RATE )
	t = np.arange( length ) / SAMPLE_RATE
	impulseResponse = reverbRng.standard_normal( length ) * np.exp( -t / decayTime )
	# Reflections lose their high frequencies, and the first few milliseconds are direct sound, not reverb
	impulseResponse = FilterInFrequencyDomain( impulseResponse, LowPassResponse( brightness, 1 ) )
	impulseResponse[:int( 0.012 * SAMPLE_RATE )] = 0.0
	impulseResponse /= np.sqrt( np.sum( impulseResponse ** 2 ) )
	paddedLength = 1 << ( ( len( signal ) + length - 1 ).bit_length() )
	wet = np.fft.irfft( np.fft.rfft( signal, paddedLength ) * np.fft.rfft( impulseResponse, paddedLength ), paddedLength )[:len( signal )]
	return signal + wetness * wet

def Fade( signal, fadeInTime = 0.002, fadeOutTime = 0.02 ):
	"""Fades the very start and end so that the sound doesn't start or stop with a click"""
	signal = signal.copy()
	fadeIn = int( fadeInTime * SAMPLE_RATE )
	fadeOut = int( fadeOutTime * SAMPLE_RATE )
	if fadeIn > 0:
		signal[:fadeIn] *= np.linspace( 0.0, 1.0, fadeIn )
	if fadeOut > 0:
		signal[-fadeOut:] *= np.linspace( 1.0, 0.0, fadeOut )
	return signal

def Normalize( signal, peak ):
	return signal * ( peak / max( np.max( np.abs( signal ) ), 1e-9 ) )

def WriteWave( fileName, signal, peak = 0.89 ):
	"""Writes a mono (1-D) or stereo ( [left, right] ) signal as a 16-bit .wav file"""
	channels = signal if isinstance( signal, tuple ) else ( signal, )
	scale = peak / max( max( np.max( np.abs( channel ) ) for channel in channels ), 1e-9 )
	interleaved = np.stack( [channel * scale for channel in channels], axis = 1 )
	samples = np.clip( np.round( interleaved * 32767.0 ), -32768, 32767 ).astype( "<i2" )
	path = os.path.join( OUTPUT_DIRECTORY, fileName )
	with wave.open( path, "wb" ) as waveFile:
		waveFile.setnchannels( len( channels ) )
		waveFile.setsampwidth( 2 )
		waveFile.setframerate( SAMPLE_RATE )
		waveFile.writeframes( samples.tobytes() )
	print( "Wrote {} ({:.2f} s, {})".format( fileName, len( channels[0] ) / SAMPLE_RATE, "stereo" if len( channels ) == 2 else "mono" ) )

def MidiToFrequency( note ):
	return 440.0 * 2.0 ** ( ( note - 69 ) / 12.0 )


# Voices (for the crowd)
#=======================

# Vowel formants: ( frequency, bandwidth, gain ) for the first three resonances of the mouth
VOWEL_AH = [( 750.0, 110.0, 1.0 ), ( 1200.0, 120.0, 0.55 ), ( 2550.0, 160.0, 0.25 )]
VOWEL_AW = [( 620.0, 100.0, 1.0 ), ( 950.0, 110.0, 0.6 ), ( 2500.0, 160.0, 0.15 )]
VOWEL_EH = [( 560.0, 100.0, 1.0 ), ( 1800.0, 130.0, 0.5 ), ( 2550.0, 160.0, 0.25 )]
VOWEL_OO = [( 380.0, 90.0, 1.0 ), ( 900.0, 110.0, 0.35 ), ( 2300.0, 160.0, 0.1 )]
VOWEL_MURMUR = [( 520.0, 160.0, 1.0 ), ( 1450.0, 220.0, 0.5 ), ( 2500.0, 250.0, 0.2 )]

def VoiceSource( pitchCurve ):
	"""A buzzing vocal-cord source that follows a pitch curve (one frequency per sample)"""
	# A little random wobble in pitch keeps many voices from sounding like one synthesizer
	jitter = FilterInFrequencyDomain( rng.standard_normal( len( pitchCurve ) ), LowPassResponse( 6.0, 1 ) )
	jitter /= max( np.std( jitter ), 1e-9 )
	phase = 2.0 * math.pi * np.cumsum( pitchCurve * ( 1.0 + 0.012 * jitter ) ) / SAMPLE_RATE
	source = np.zeros( len( pitchCurve ) )
	highestPitch = np.max( pitchCurve )
	harmonicCount = int( 4500.0 / highestPitch )
	for harmonic in range( 1, harmonicCount + 1 ):
		# The vocal cords' harmonics get quieter at about 12 dB per octave
		source += np.sin( harmonic * phase ) / ( harmonic ** 1.6 )
	return source

def Crowd( duration, voiceCount, vowel, PitchCurveForVoice, EnvelopeForVoice, breathiness = 0.25 ):
	"""Many voices shaped by the same vowel (the filter is applied once to the sum, which is the same as filtering each voice)"""
	total = np.zeros( int( duration * SAMPLE_RATE ) )
	for voice in range( voiceCount ):
		pitchCurve = PitchCurveForVoice( voice )
		envelope = EnvelopeForVoice( voice )
		source = VoiceSource( pitchCurve )
		# Shouting is partly breath noise
		source += breathiness * rng.standard_normal( len( source ) ) * 0.4
		total += source * envelope * rng.uniform( 0.5, 1.0 )
	return FilterInFrequencyDomain( total, FormantResponse( vowel ) )

def Applause( duration, clapperCount, DensityAt, seed ):
	"""Many people clapping: each clap is a few milliseconds of band-passed noise"""
	applauseRng = np.random.default_rng( seed )
	total = np.zeros( int( duration * SAMPLE_RATE ) )
	clapLength = int( 0.03 * SAMPLE_RATE )
	clapTimes = np.arange( clapLength ) / SAMPLE_RATE
	for _ in range( clapperCount ):
		# Everyone's hands sound a little different
		clap = applauseRng.standard_normal( clapLength ) * np.exp( -clapTimes / applauseRng.uniform( 0.003, 0.007 ) )
		clap = FilterInFrequencyDomain( clap, BandPassResponse( applauseRng.uniform( 900.0, 2600.0 ), applauseRng.uniform( 0.8, 1.6 ) ) )
		loudness = applauseRng.uniform( 0.3, 1.0 )
		rate = applauseRng.uniform( 3.5, 6.0 )	# claps per second
		time = applauseRng.uniform( 0.0, 0.25 )
		while time < duration:
			if applauseRng.uniform() < DensityAt( time ):
				AddAt( total, clap * loudness * applauseRng.uniform( 0.7, 1.0 ), time )
			time += ( 1.0 / rate ) * applauseRng.uniform( 0.85, 1.15 )
	return total


# Brass (for the fanfare)
#========================

def BrassNote( frequency, duration, loudness = 1.0, attackTime = 0.05, releaseTime = 0.12 ):
	"""A brass note: harmonics whose brightness follows the note's loudness"""
	t = Times( duration + releaseTime )
	totalTime = duration + releaseTime
	envelope = Envelope( totalTime, [( 0.0, 0.0 ), ( attackTime, 1.0 ), ( attackTime + 0.08, 0.82 ),
		( duration, 0.75 ), ( totalTime, 0.0 )] ) * loudness
	# Vibrato starts a little after the note does, the way a player adds it
	vibrato = 1.0 + 0.004 * np.sin( 2.0 * math.pi * 5.2 * t ) * np.clip( ( t - 0.25 ) / 0.3, 0.0, 1.0 )
	phase = 2.0 * math.pi * np.cumsum( frequency * vibrato ) / SAMPLE_RATE
	note = np.zeros_like( t )
	harmonicCount = min( 24, int( 9000.0 / frequency ) )
	brightness = 1.5 + 6.0 * envelope
	for harmonic in range( 1, harmonicCount + 1 ):
		note += np.exp( -( harmonic - 1 ) / brightness ) * np.sin( harmonic * phase )
	# The "blat" at the start of a brass note: a short burst of breath
	note += 0.15 * Pad( Noise( attackTime ) * np.linspace( 1.0, 0.0, int( attackTime * SAMPLE_RATE ) ), totalTime )
	return note * envelope

def Timpani( frequency, loudness = 1.0, duration = 2.2 ):
	t = Times( duration )
	# A drum's pitch drops slightly as the head stops vibrating so hard
	pitch = frequency * ( 1.0 + 0.04 * np.exp( -t / 0.08 ) )
	phase = 2.0 * math.pi * np.cumsum( pitch ) / SAMPLE_RATE
	body = ( np.sin( phase ) + 0.5 * np.sin( 1.5 * phase ) * np.exp( -t / 0.25 ) + 0.25 * np.sin( 1.99 * phase ) * np.exp( -t / 0.4 ) )
	body *= np.exp( -t / 0.7 )
	stick = FilterInFrequencyDomain( Noise( duration ) * np.exp( -t / 0.012 ), LowPassResponse( 1800.0 ) )
	return loudness * ( body + 0.6 * stick )

def Cymbal( duration, swellTime ):
	t = Times( duration )
	envelope = np.where( t < swellTime, ( t / swellTime ) ** 2, np.exp( -( t - swellTime ) / 0.9 ) )
	return FilterInFrequencyDomain( Noise( duration ), HighPassResponse( 5000.0, 2 ) ) * envelope


# The Sounds
#===========

def MakeBowRelease():
	duration = 0.9
	t = Times( duration )
	# The string: Karplus-Strong (a burst of noise circulating through a delay of one period, losing a little each time)
	stringFrequency = 92.0
	period = int( SAMPLE_RATE / stringFrequency )
	string = np.zeros( len( t ) )
	string[:period] = rng.uniform( -1.0, 1.0, period )
	for i in range( period, len( string ) ):
		string[i] = 0.493 * ( string[i - period] + string[i - period - 1] )
	string *= np.exp( -t / 0.12 )
	# The "thwack" of the string hitting the arm guard
	snap = FilterInFrequencyDomain( Noise( duration ) * np.exp( -t / 0.006 ), BandPassResponse( 1600.0, 0.9 ) )
	# The bow's limbs thumping forward
	thump = DecayingSine( 58.0, 0.06, duration ) + 0.5 * DecayingSine( 116.0, 0.03, duration )
	# The arrow leaving: noise swept from high to low as it flies away
	whooshEnvelope = Envelope( duration, [( 0.0, 0.0 ), ( 0.03, 1.0 ), ( 0.12, 0.6 ), ( 0.45, 0.0 ), ( duration, 0.0 )] )
	whoosh = np.zeros( len( t ) )
	noise = Noise( duration )
	for center, start, end in ( ( 3200.0, 0.0, 0.08 ), ( 2200.0, 0.06, 0.2 ), ( 1400.0, 0.16, 0.45 ) ):
		band = FilterInFrequencyDomain( noise, BandPassResponse( center, 2.0 ) )
		window = Envelope( duration, [( 0.0, 0.0 ), ( start, 0.0 ), ( ( start + end ) / 2, 1.0 ), ( end, 0.0 ), ( duration, 0.0 )] )
		whoosh += band * window
	whoosh *= whooshEnvelope
	sound = 0.7 * string / np.max( np.abs( string ) ) + 0.8 * snap / np.max( np.abs( snap ) ) + 0.9 * thump + 0.45 * whoosh / np.max( np.abs( whoosh ) )
	WriteWave( "bowRelease.wav", Fade( Reverb( sound, 0.25, 0.12, seed = 1 ) ) )

def ArrowQuiver( duration, frequency, rate, decayTime ):
	"""The shaft of an arrow wobbling after it sticks into something"""
	t = Times( duration )
	return np.sin( 2.0 * math.pi * frequency * t ) * np.abs( np.sin( 2.0 * math.pi * rate * t ) ) * np.exp( -t / decayTime )

def MakeObstacleHit():
	duration = 0.9
	t = Times( duration )
	# The moment the tip hits the board: a sharp click
	click = FilterInFrequencyDomain( Noise( duration ) * np.exp( -t / 0.0025 ), HighPassResponse( 1500.0 ) )
	# The board ringing: the frequencies that a plank of wood resonates at, each dying away at its own rate
	modes = ( ( 185.0, 0.07, 1.0 ), ( 405.0, 0.05, 0.7 ), ( 690.0, 0.035, 0.55 ), ( 1130.0, 0.022, 0.4 ), ( 2250.0, 0.012, 0.25 ) )
	wood = sum( amplitude * DecayingSine( frequency, decayTime, duration, rng.uniform( 0.0, 6.28 ) ) for frequency, decayTime, amplitude in modes )
	quiver = ArrowQuiver( duration, 310.0, 16.0, 0.22 )
	sound = 0.8 * click / np.max( np.abs( click ) ) + wood / np.max( np.abs( wood ) ) + 0.18 * quiver
	WriteWave( "obstacleHit.wav", Fade( Reverb( sound, 0.35, 0.18, seed = 2 ) ) )

def MakeTargetHit():
	duration = 0.8
	t = Times( duration )
	# A dull thump: the straw boss is soft and heavy, so there is no ringing
	pitch = 75.0 * ( 1.0 + 0.5 * np.exp( -t / 0.02 ) )
	thump = np.sin( 2.0 * math.pi * np.cumsum( pitch ) / SAMPLE_RATE ) * np.exp( -t / 0.07 )
	# The straw crunching
	crunch = FilterInFrequencyDomain( Noise( duration ) * np.exp( -t / 0.035 ), BandPassResponse( 900.0, 0.7 ) )
	quiver = ArrowQuiver( duration, 280.0, 14.0, 0.25 )
	sound = thump + 0.55 * crunch / np.max( np.abs( crunch ) ) + 0.14 * quiver
	WriteWave( "targetHit.wav", Fade( Reverb( sound, 0.3, 0.12, seed = 3 ) ) )

def Bell( frequency, duration, decayTime ):
	t = Times( duration )
	# A bell's partials aren't whole-number multiples of its pitch, which is what makes it sound like a bell
	partials = ( ( 0.5, 0.5, 1.6 ), ( 1.0, 1.0, 1.0 ), ( 1.19, 0.45, 0.8 ), ( 1.5, 0.35, 0.6 ), ( 2.0, 0.55, 0.5 ), ( 2.74, 0.25, 0.35 ), ( 3.76, 0.15, 0.25 ) )
	bell = np.zeros_like( t )
	for ratio, amplitude, decayScale in partials:
		bell += amplitude * np.sin( 2.0 * math.pi * frequency * ratio * t ) * np.exp( -t / ( decayTime * decayScale ) )
	# The strike
	bell += 0.3 * FilterInFrequencyDomain( Noise( duration ) * np.exp( -t / 0.002 ), HighPassResponse( 3000.0 ) )
	return bell

def MakeBullseye():
	duration = 2.4
	sound = np.zeros( int( duration * SAMPLE_RATE ) )
	# Two rising chimes, like a stadium's "you did it" sound
	# (each bell lasts until the end of the sound so that none of them is cut off while it is still ringing)
	AddAt( sound, Bell( MidiToFrequency( 88 ), duration, 0.45 ), 0.0 )	# E6
	AddAt( sound, Bell( MidiToFrequency( 93 ), duration - 0.16, 0.55 ), 0.16 )	# A6
	WriteWave( "bullseye.wav", Fade( Reverb( sound, 0.9, 0.3, brightness = 7000.0, seed = 4 ), fadeOutTime = 0.4 ) )

def MakeFanfare():
	"""An original ceremonial fanfare for the opening shot"""
	beat = 0.42
	duration = 9.0
	melody = np.zeros( int( duration * SAMPLE_RATE ) )
	harmony = np.zeros_like( melody )
	bass = np.zeros_like( melody )
	drums = np.zeros_like( melody )
	# ( MIDI note, start in beats, length in beats ): G4 G4 | C5. E5 G5 | A5 G5 E5 F5 D5 | C5 (held)
	melodyNotes = ( ( 67, 0.0, 0.45 ), ( 67, 0.5, 0.45 ), ( 72, 1.0, 1.4 ), ( 76, 2.5, 0.45 ), ( 79, 3.0, 1.9 ),
		( 81, 5.0, 0.9 ), ( 79, 6.0, 0.45 ), ( 76, 6.5, 0.45 ), ( 77, 7.0, 0.9 ), ( 74, 8.0, 0.9 ), ( 72, 9.0, 6.0 ) )
	harmonyNotes = ( ( 64, 1.0, 1.4 ), ( 67, 2.5, 0.45 ), ( 72, 3.0, 1.9 ), ( 77, 5.0, 0.9 ), ( 76, 6.0, 0.45 ), ( 72, 6.5, 0.45 ),
		( 74, 7.0, 0.9 ), ( 71, 8.0, 0.9 ), ( 64, 9.0, 6.0 ), ( 67, 9.0, 6.0 ) )
	bassNotes = ( ( 48, 1.0, 3.9 ), ( 53, 5.0, 1.9 ), ( 55, 7.0, 1.9 ), ( 48, 9.0, 6.0 ), ( 36, 9.0, 6.0 ) )
	for notes, track, loudness in ( ( melodyNotes, melody, 1.0 ), ( harmonyNotes, harmony, 0.6 ), ( bassNotes, bass, 0.7 ) ):
		for note, start, length in notes:
			AddAt( track, BrassNote( MidiToFrequency( note ), length * beat, loudness ), start * beat )
	for frequency, start, loudness in ( ( 65.4, 1.0, 1.0 ), ( 49.0, 3.0, 0.7 ), ( 65.4, 5.0, 0.8 ), ( 49.0, 7.0, 0.8 ) ):
		AddAt( drums, Timpani( frequency, loudness ), start * beat )
	# A timpani roll swelling into the last chord
	rollStart = 8.0
	for i in range( 16 ):
		AddAt( drums, Timpani( 65.4, 0.25 + 0.04 * i, 1.4 ), ( rollStart + i * 0.0625 ) * beat )
	AddAt( drums, Timpani( 65.4, 1.2 ), 9.0 * beat )
	AddAt( drums, 0.35 * Cymbal( 4.0, 0.08 ), 9.0 * beat - 0.08 )
	mix = melody + 0.8 * harmony + 0.9 * bass + 0.55 * drums / np.max( np.abs( drums ) ) * np.max( np.abs( melody ) )
	# The last chord swells a little and then fades
	mix *= Envelope( duration, [( 0.0, 1.0 ), ( 9.0 * beat, 1.0 ), ( 9.0 * beat + 0.6, 1.15 ), ( duration - 2.0, 0.8 ), ( duration, 0.0 )] )
	# Brass played outdoors and heard from a distance isn't as bright as up close
	mix = FilterInFrequencyDomain( mix, LowPassResponse( 4500.0, 1 ) )
	mix = Reverb( mix, 1.1, 0.35, brightness = 5000.0, seed = 5 )
	WriteWave( "introFanfare.wav", Fade( mix, fadeOutTime = 0.3 ) )

def MakeRoundComplete():
	"""A short flourish for the end of a round"""
	beat = 0.16
	duration = 3.0
	mix = np.zeros( int( duration * SAMPLE_RATE ) )
	for note, start, length, loudness in ( ( 72, 0, 0.9, 0.8 ), ( 76, 1, 0.9, 0.85 ), ( 79, 2, 0.9, 0.9 ), ( 84, 3, 8.0, 1.0 ),
		( 76, 3, 8.0, 0.55 ), ( 79, 3, 8.0, 0.55 ), ( 60, 3, 8.0, 0.6 ), ( 48, 3, 8.0, 0.5 ) ):
		AddAt( mix, BrassNote( MidiToFrequency( note ), length * beat, loudness ), start * beat )
	AddAt( mix, 0.5 * Timpani( 65.4, 1.0 ) * np.max( np.abs( mix ) ), 3 * beat )
	mix *= Envelope( duration, [( 0.0, 1.0 ), ( 1.4, 1.0 ), ( duration, 0.0 )] )
	mix = FilterInFrequencyDomain( mix, LowPassResponse( 4500.0, 1 ) )
	WriteWave( "roundComplete.wav", Fade( Reverb( mix, 0.9, 0.3, seed = 6 ), fadeOutTime = 0.2 ) )

def RandomPitch( low, high ):
	# A mix of lower and higher voices
	return rng.uniform( low, high )

def MakeCheer():
	duration = 5.0
	t = Times( duration )
	channels = []
	for side in range( 2 ):
		# Shouting voices: each starts a moment after the hit, rises in pitch ("whoo!"), and holds
		def PitchCurve( voice ):
			start = rng.uniform( 180.0, 330.0 )
			rise = rng.uniform( 1.15, 1.5 )
			riseTime = rng.uniform( 0.2, 0.5 )
			return start * ( 1.0 + ( rise - 1.0 ) * np.clip( t / riseTime, 0.0, 1.0 ) ) * ( 1.0 + 0.03 * np.sin( 2 * math.pi * rng.uniform( 3, 6 ) * t ) )
		def VoiceEnvelope( voice ):
			start = rng.uniform( 0.0, 0.35 )
			end = rng.uniform( 1.6, 3.8 )
			return Envelope( duration, [( 0.0, 0.0 ), ( start, 0.0 ), ( start + 0.12, 1.0 ), ( end, 0.7 ), ( end + 0.6, 0.0 ), ( duration, 0.0 )] )
		whoo = Crowd( duration, 28, VOWEL_OO, PitchCurve, VoiceEnvelope, breathiness = 0.4 )
		yeah = Crowd( duration, 28, VOWEL_EH, PitchCurve, VoiceEnvelope, breathiness = 0.4 )
		ah = Crowd( duration, 24, VOWEL_AH, PitchCurve, VoiceEnvelope, breathiness = 0.4 )
		voices = whoo + yeah + ah
		applause = Applause( duration, 70, lambda time: float( np.interp( time, [0.0, 0.2, 2.5, 4.6], [0.0, 1.0, 0.85, 0.0] ) ), seed = 10 + side )
		mix = voices / np.max( np.abs( voices ) ) + 0.75 * applause / np.max( np.abs( applause ) )
		# The crowd is across the field, so the highest frequencies are softened
		mix = FilterInFrequencyDomain( mix, LowPassResponse( 5500.0, 1 ) )
		channels.append( Reverb( mix, 0.8, 0.35, seed = 20 + side ) )
	WriteWave( "crowdCheer.wav", tuple( Fade( channel, 0.01, 0.4 ) for channel in channels ) )

def MakeGroan():
	duration = 3.2
	t = Times( duration )
	channels = []
	for side in range( 2 ):
		# "Awww": every voice falls in pitch, the way people's voices drop when they're disappointed
		def PitchCurve( voice ):
			start = rng.uniform( 170.0, 320.0 )
			fall = rng.uniform( 0.62, 0.75 )
			return start * ( fall + ( 1.0 - fall ) * np.exp( -t / rng.uniform( 0.5, 0.9 ) ) )
		def VoiceEnvelope( voice ):
			start = rng.uniform( 0.0, 0.25 )
			end = rng.uniform( 1.4, 2.2 )
			return Envelope( duration, [( 0.0, 0.0 ), ( start, 0.0 ), ( start + 0.2, 1.0 ), ( end, 0.6 ), ( end + 0.7, 0.0 ), ( duration, 0.0 )] )
		aw = Crowd( duration, 40, VOWEL_AW, PitchCurve, VoiceEnvelope, breathiness = 0.3 )
		oo = Crowd( duration, 20, VOWEL_OO, PitchCurve, VoiceEnvelope, breathiness = 0.3 )
		mix = aw / np.max( np.abs( aw ) ) + 0.5 * oo / np.max( np.abs( oo ) )
		mix = FilterInFrequencyDomain( mix, LowPassResponse( 4500.0, 1 ) )
		channels.append( Reverb( mix, 0.8, 0.35, seed = 30 + side ) )
	WriteWave( "crowdGroan.wav", tuple( Fade( channel, 0.01, 0.4 ) for channel in channels ) )

def BirdChirp( duration ):
	"""A small bird's trill: quick upward sweeps of a whistle"""
	t = Times( duration )
	chirpCount = int( rng.integers( 3, 7 ) )
	chirpLength = 0.07
	base = rng.uniform( 3200.0, 4600.0 )
	trill = np.zeros_like( t )
	for i in range( chirpCount ):
		chirpTime = Times( chirpLength )
		sweep = base * ( 1.0 + 0.35 * chirpTime / chirpLength )
		chirp = np.sin( 2.0 * math.pi * np.cumsum( sweep ) / SAMPLE_RATE ) * np.sin( math.pi * chirpTime / chirpLength ) ** 2
		AddAt( trill, chirp, i * 0.095 )
	return trill

def MakeCrowdAmbience():
	"""A loop of a waiting crowd murmuring on a calm morning, with a little wind and the odd bird"""
	loopDuration = 12.0
	crossfadeDuration = 1.5
	duration = loopDuration + crossfadeDuration
	t = Times( duration )
	channels = []
	for side in range( 2 ):
		# Murmuring: voices talking in syllables (each voice turns on and off a few times a second)
		def PitchCurve( voice ):
			base = rng.uniform( 110.0, 260.0 )
			wander = FilterInFrequencyDomain( rng.standard_normal( len( t ) ), LowPassResponse( 2.0, 1 ) )
			return base * ( 1.0 + 0.08 * wander / max( np.std( wander ), 1e-9 ) )
		def VoiceEnvelope( voice ):
			syllables = FilterInFrequencyDomain( rng.standard_normal( len( t ) ), BandPassResponse( rng.uniform( 3.0, 5.0 ), 1.0 ) )
			syllables = np.clip( syllables / max( np.std( syllables ), 1e-9 ), 0.0, None )
			# People talk in sentences with pauses between them
			sentences = FilterInFrequencyDomain( rng.standard_normal( len( t ) ), LowPassResponse( 0.25, 1 ) )
			sentences = np.clip( sentences / max( np.std( sentences ), 1e-9 ) + 0.3, 0.0, 1.0 )
			return syllables * sentences
		murmur = Crowd( duration, 40, VOWEL_MURMUR, PitchCurve, VoiceEnvelope, breathiness = 0.15 )
		murmur = FilterInFrequencyDomain( murmur, LowPassResponse( 2500.0, 2 ) )
		murmur /= np.max( np.abs( murmur ) )
		wind = FilterInFrequencyDomain( Noise( duration ), LowPassResponse( 400.0, 2 ) )
		wind *= 0.6 + 0.4 * np.sin( 2.0 * math.pi * t / loopDuration * 2.0 + side )
		wind /= np.max( np.abs( wind ) )
		birds = np.zeros_like( t )
		for chirpTime in rng.uniform( 0.5, loopDuration - 1.0, 3 ):
			AddAt( birds, BirdChirp( 0.7 ), chirpTime )
		mix = Reverb( 0.7 * murmur + 0.25 * wind, 0.9, 0.3, seed = 40 + side )
		# The birds are added after the reverb: they are out in the open, not bouncing around the stands
		mix += 0.06 * birds
		# Make the end flow into the start: the last part is faded into the first part,
		# so when the loop jumps from the end back to the start there is no jump in the sound
		crossfade = int( crossfadeDuration * SAMPLE_RATE )
		loop = mix[:int( loopDuration * SAMPLE_RATE )].copy()
		ramp = np.linspace( 0.0, 1.0, crossfade )
		loop[:crossfade] = loop[:crossfade] * np.sqrt( ramp ) + mix[-crossfade:] * np.sqrt( 1.0 - ramp )
		channels.append( loop )
	WriteWave( "crowdAmbience.wav", tuple( channels ), peak = 0.7 )


def MakeWhistle():
	"""
		The referee's whistle that means "you may shoot" (one blast starts an end in real archery).
		A pea whistle is a high tone with a fast warble, made by the little ball (the pea) spinning around inside it
		and interrupting the air flow, plus a little breath noise.
	"""
	duration = 0.75
	t = Times( duration )
	envelope = Envelope( duration, [( 0.0, 0.0 ), ( 0.03, 1.0 ), ( 0.4, 0.9 ), ( 0.48, 0.0 ), ( duration, 0.0 )] )
	warbleRate = 38.0	# times per second that the pea goes around
	warble = np.sin( 2.0 * math.pi * warbleRate * t )
	pitch = 3050.0 * ( 1.0 + 0.02 * warble )
	phase = 2.0 * math.pi * np.cumsum( pitch ) / SAMPLE_RATE
	tone = ( np.sin( phase ) + 0.15 * np.sin( 2.0 * phase ) ) * ( 0.65 + 0.35 * ( 0.5 + 0.5 * warble ) )
	breath = FilterInFrequencyDomain( Noise( duration ), BandPassResponse( 3000.0, 3.0 ) )
	sound = ( tone + 0.25 * breath / np.max( np.abs( breath ) ) ) * envelope
	WriteWave( "whistle.wav", Fade( Reverb( sound, 0.4, 0.15, brightness = 6000.0, seed = 7 ), fadeOutTime = 0.1 ) )


if __name__ == "__main__":
	MakeBowRelease()
	MakeObstacleHit()
	MakeTargetHit()
	MakeBullseye()
	MakeFanfare()
	MakeRoundComplete()
	MakeCheer()
	MakeGroan()
	MakeCrowdAmbience()
	# (Made last so that adding it didn't change the random numbers, and so the sound, of anything made before it)
	MakeWhistle()
