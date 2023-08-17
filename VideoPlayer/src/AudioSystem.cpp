#include "AudioSystem.h"

AudioSystem::AudioSystem()
{
	this->dir = "res/";

	// Set audio device as default.
	this->device = alcOpenDevice(nullptr);
	if (!this->device)
	{
		std::cout << "ERROR::OpenAL: Default audio device not found." << std::endl;
	}

	// Create OpenAL context.
	this->context = alcCreateContext(device, nullptr);
	if (!this->context)
	{
		std::cout << "ERROR::OpenAL: Failed to create OpenAL context." << std::endl;
	}

	// Set the new OpenAL context as current.
	if (!alcMakeContextCurrent(context))
	{
		std::cout << "ERROR::OpenAL: Audio context could not be made current." << std::endl;
	}

	alGenBuffers(1, &this->buffer);
	alGenSources(1, &this->source);

	ALint value;
	alGetSourcei(this->source, AL_SOURCE_STATE, &value);
	if (value == AL_PLAYING)
	{
		std::cout << "Uh oh, how did we get here?" << std::endl;
	}

	//alSourcef(this->source, AL_PITCH, 1.0f);
	//alSourcef(this->source, AL_GAIN, 1.0f);
	//alSourcef(this->source, AL_LOOPING, true);
}

AudioSystem::~AudioSystem()
{
	delete(this->clip);

	alSourceStop(this->source);
	alDeleteSources(1, &this->source);
	alDeleteBuffers(1, &this->buffer);

	alcMakeContextCurrent(nullptr);
	alcDestroyContext(this->context);
	alcCloseDevice(this->device);
}

bool AudioSystem::Load(std::string fileName)
{
	/*
	std::string filePath = this->dir + fileName;

	// Load WAV file from disk.
	std::ifstream file(filePath, std::ios::binary);
	if (file.bad())
	{
		std::cout << "ERROR::OpenAL: Bad audio file path." << std::endl;
		return false;
	}

	// Read WAV file header.
	WavHeader header;
	file.read((char*)&header, sizeof(header));

	// Loop until the data header is found.
	while (header.DATA[0] != 'D' && header.DATA[0] != 'd')
	{
		// Erase old data.
		char* buf = new char[header.dataSize];
		file.read(&buf[0], header.dataSize);
		delete[](buf);

		// Read new data.
		char buffer[4];
		file.read(buffer, 4);
		header.DATA[0] = buffer[0];
		header.DATA[1] = buffer[1];
		header.DATA[2] = buffer[2];
		header.DATA[3] = buffer[3];
		file.read(buffer, 4);

		// Copy the data to header.
		std::int32_t temp = 0;
		std::memcpy(&temp, buffer, 4);
		header.dataSize = temp;

		// Test for end of file to prevent infinite loop.
		if (file.eof())
		{
			return false;
		}
	}

	// Print the WAV header data for debugging.
	std::cout << "Filename: " << fileName << std::endl;
	header.summary();

	// Copy relevant header data to AudioClip
	uint32_t frequency = header.frequency;

	// Read wav file data
	char* data = new char[header.dataSize];
	file.read(&data[0], header.dataSize);

	// Determine audio format from header
	ALenum format;
	if (header.channels == 1 && header.bitDepth == 8)
		format = AL_FORMAT_MONO8;
	else if (header.channels == 1 && header.bitDepth == 16)
		format = AL_FORMAT_MONO16;
	else if (header.channels == 2 && header.bitDepth == 8)
		format = AL_FORMAT_STEREO8;
	else if (header.channels == 2 && header.bitDepth == 16)
		format = AL_FORMAT_STEREO16;
	else
	{
		std::cout << "ERROR: unreadable wave format: "
			<< header.channels << " channels, "
			<< header.bitDepth << " bps.\n";
		return false;
	}

	this->clip = new AudioClip(format, frequency, filePath);

	alGenBuffers(1, &this->clip->buffer);
	alBufferData(this->clip->buffer, format, data, header.dataSize, header.frequency);

	// Unload WAV from RAM.
	delete[](data);
	*/

	return true;
}

void AudioSystem::Play(uint8_t* data, int size, int sampleRate)
{
	ALint value;
	//alGetSourcei(this->source, AL_SOURCE_STATE, &value);
	//if (value == AL_PLAYING)
	//{
	//	return;
	//}
	//else
	//{
	//	std::cout << "Source state: " << value << std::endl;
	//}

	alSourceStop(this->source);
	alSourcei(this->source, AL_BUFFER, 0);

	alBufferData(this->buffer, AL_FORMAT_MONO8, data, size, sampleRate);

	alSourcei(this->source, AL_BUFFER, this->buffer);
	alSourcePlay(this->source);
}