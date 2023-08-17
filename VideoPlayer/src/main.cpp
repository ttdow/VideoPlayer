#define STB_IMAGE_IMPLEMENTATION

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <stbi/stb_image.h>

#include "AudioSystem.h"

#include <iostream>

extern "C" {
#include <libavformat/avformat.h>
#include <libavutil/imgutils.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
#include <libavutil/channel_layout.h>
#include <libswresample/swresample.h>
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
	glViewport(0, 0, width, height);
}

void processInput(GLFWwindow* window)
{
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
	{
		glfwSetWindowShouldClose(window, true);
	}
}

int main()
{
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	GLFWwindow* window = glfwCreateWindow(800, 600, "VideoPlayer", NULL, NULL);
	if (window == NULL)
	{
		std::cout << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		return -1;
	}

	glfwMakeContextCurrent(window);
	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		std::cout << "Failed to initialize GLAD" << std::endl;
		return -1;
	}

	glViewport(0, 0, 800, 600);

	// Top-level structure representing the multimedia file. It contains
	// information about the file format, file-level metadata, and pointers
	// to individual streams.
	// AVFormat Context

	// Initialize FFmpeg.
	avformat_network_init();

	// Open the multimedia file.
	// Holds information about a multimedia stream's format and provides a context
	// for reading or writing multimedia files.
	AVFormatContext* formatContext = nullptr;

	// Open a multimedia file and initialize the AVFormatContext structure to
	// represent that file. Sets up the context with information about the
	// multimedia file's format, codecs, and streams.
	if (avformat_open_input(&formatContext, "file:res/video.mp4", nullptr, nullptr) != 0)
	{
		std::cout << "ERROR::FFMPEG: Couldn't open file." << std::endl;
		return -1;
	}

	// Find stream information.
	if (avformat_find_stream_info(formatContext, nullptr))
	{
		std::cout << "ERROR::FFMPEG: Couldn't find stream information." << std::endl;
		avformat_close_input(&formatContext);
		return -1;
	}

	// Loop through all of the streams to find the video or audio stream.
	int videoStreamIndex = -1;
	int audioStreamIndex = -1;
	for (int i = 0; i < formatContext->nb_streams; i++)
	{
		if (formatContext->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
		{
			videoStreamIndex = i;
		}

		if (formatContext->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO)
		{
			audioStreamIndex = i;
		}
	}

	// Ensure a viable video stream was found.
	if (videoStreamIndex == -1)
	{
		std::cout << "ERROR::FFMPEG: No video stream found." << std::endl;
		avformat_close_input(&formatContext);
		return -1;
	}

	// Ensure a viable audio stream was found.
	if (audioStreamIndex == -1)
	{
		std::cout << "ERROR::FMPEG: No audio stream found." << std::endl;
		avformat_close_input(&formatContext);
		return -1;
	}
	
	// ------------------------- AUDIO ----------------------------------------

	// Get the audio codec parameters.
	AVCodecParameters* audioCodecParams = formatContext->streams[audioStreamIndex]->codecpar;
	AVCodecContext* audioCodecContext = avcodec_alloc_context3(nullptr);
	if (avcodec_parameters_to_context(audioCodecContext, audioCodecParams) < 0)
	{
		std::cout << "ERROR::FFMPEG: Failed to fill codec context from codec parameters." << std::endl;
		avformat_close_input(&formatContext);
		return -1;
	}

	// Find the audio codec.
	const AVCodec* audioCodec = avcodec_find_decoder(audioCodecContext->codec_id);
	if (!audioCodec)
	{
		std::cout << "ERROR::FFMPEG: Codec not found." << std::endl;
		avcodec_free_context(&audioCodecContext);
		avformat_close_input(&formatContext);
		return -1;
	}

	// Open the audio codec.
	if (avcodec_open2(audioCodecContext, audioCodec, nullptr))
	{
		std::cout << "ERROR::FMPEG: Error opening the codec." << std::endl;
		avcodec_free_context(&audioCodecContext);
		avformat_close_input(&formatContext);
		return -1;
	}

	// ------------------------- VIDEO ----------------------------------------

	// Get the video codec context.
	AVCodecParameters* videoCodecParams = formatContext->streams[videoStreamIndex]->codecpar;
	AVCodecContext* videoCodecContext = avcodec_alloc_context3(nullptr);
	if (avcodec_parameters_to_context(videoCodecContext, videoCodecParams) < 0)
	{
		std::cout << "ERROR::FFMPEG: Failed to fill codec context from codec parameters." << std::endl;
		avformat_close_input(&formatContext);
		return -1;
	}

	// Find the video codec.
	const AVCodec* codec = avcodec_find_decoder(videoCodecContext->codec_id);
	if (!codec)
	{
		std::cout << "ERROR::FFMPEG: Codec not found." << std::endl;
		avcodec_free_context(&videoCodecContext);
		avformat_close_input(&formatContext);
		return -1;
	}

	// Open the video codec.
	if (avcodec_open2(videoCodecContext, codec, nullptr))
	{
		std::cout << "ERROR::FMPEG: Error opening the codec." << std::endl;
		avcodec_free_context(&videoCodecContext);
		avformat_close_input(&formatContext);
		return -1;
	}

	// Setup video frame and packet.
	AVFrame* frame = av_frame_alloc();
	if (frame == nullptr)
	{
		std::cout << "ERROR::FFMPEG: Frame was not properly allocated." << std::endl;
	}

	AVPacket packet = { 0 };

	const char* vertexShaderSource = "#version 330 core\n"
		"layout (location = 0) in vec3 aPos;\n"
		"layout (location = 1) in vec2 aTexCoord;\n"
		"out vec2 TexCoord;\n"
		"void main()\n"
		"{\n"
		"	TexCoord = aTexCoord;\n"
		"   gl_Position = vec4(aPos.x, aPos.y, aPos.z, 1.0);\n"
		"}\0";

	unsigned int vertexShader;
	vertexShader = glCreateShader(GL_VERTEX_SHADER);

	glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
	glCompileShader(vertexShader);

	const char* fragmentShaderSource = "#version 330 core\n"
		"out vec4 FragColor;\n"
		"in vec2 TexCoord;\n"
		"uniform sampler2D ourTexture;\n"
		"void main()\n"
		"{\n"
		"	FragColor = texture(ourTexture, TexCoord);\n"
		//"	FragColor = vec4(1.0f, 0.5f, 0.2f, 1.0f);\n"
		"}\n\0";

	unsigned int fragmentShader;
	fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);

	glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
	glCompileShader(fragmentShader);

	unsigned int shaderProgram;
	shaderProgram = glCreateProgram();

	glAttachShader(shaderProgram, vertexShader);
	glAttachShader(shaderProgram, fragmentShader);
	glLinkProgram(shaderProgram);

	glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader);

	float vertices[] = 
	{
		// Position				// Texture Coords
		-0.5f, -0.5f, 0.0f,		1.0f, 1.0f,
		 0.5f, -0.5f, 0.0f,		0.0f, 1.0f,
		 0.5f,  0.5f, 0.0f,		0.0f, 0.0f,
		 0.5f,  0.5f, 0.0f,		0.0f, 0.0f,
		-0.5f,  0.5f, 0.0f,		1.0f, 0.0f,
		-0.5f, -0.5f, 0.0f,		1.0f, 1.0f
	};

	unsigned int textureID;
	glGenTextures(1, &textureID);
	glBindTexture(GL_TEXTURE_2D, textureID);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	glBindTexture(GL_TEXTURE_2D, 0);

	unsigned int VAO, VBO;
	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);

	glBindVertexArray(VAO);

	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
	glEnableVertexAttribArray(1);
	
	glBindVertexArray(0);

	unsigned int counter = 0;

	AudioSystem audioSystem;

	

	while (!glfwWindowShouldClose(window))
	{
		processInput(window);

		if (av_read_frame(formatContext, &packet) >= 0)
		{
			if (packet.stream_index == audioStreamIndex)
			{
				avcodec_send_packet(audioCodecContext, &packet);

				av_frame_unref(frame);

				if (avcodec_receive_frame(audioCodecContext, frame) >= 0)
				{
					int nChannels = frame->ch_layout.nb_channels;
					int nSamples = frame->nb_samples;

					std::cout << "Channels: " << nChannels << ", Samples: " << nSamples << std::endl;

					if (frame->format == AV_SAMPLE_FMT_FLTP)
					{
						int sampleRate = frame->sample_rate;
						
						struct SwrContext* test = swr_alloc();

						swr_alloc_set_opts2(
							&test,
							&frame->ch_layout,
							AV_SAMPLE_FMT_S16,
							sampleRate,
							&frame->ch_layout,
							AV_SAMPLE_FMT_FLTP,
							sampleRate,
							0, NULL
						);

						if (swr_init(test) < 0)
						{
							std::cout << "ERROR::FMPEG: Failed to initialize swr context." << std::endl;
						}

						uint8_t** input_data = NULL;
						int in_linesize;
						int in_samples = frame->nb_samples;

						uint8_t** output_data = NULL;
						int out_linesize;
						int out_samples = frame->nb_samples;

						av_samples_alloc_array_and_samples(
							&input_data, &in_linesize, nChannels,
							in_samples, AV_SAMPLE_FMT_FLTP, 0
						);

						av_samples_alloc_array_and_samples(
							&output_data, &out_linesize, nChannels,
							out_samples, AV_SAMPLE_FMT_S16, 0
						);

						int ret = swr_convert(test, output_data, out_samples, (const uint8_t**)input_data, in_samples);

						if (ret < 0)
						{
							std::cout << "ERROR::FFMPEG: swr did not convert successfully." << std::endl;
						}
						else
						{
							std::cout << "ret = " << ret << std::endl;
							std::cout << "out_samples = " << out_samples << std::endl;
						}

						std::cout << output_data[0][0] << std::endl;

						//int size = frame->linesize[0];

						//std::cout << "sampleRate = " << sampleRate << std::endl;

						audioSystem.Play(output_data[0], ret, sampleRate);
					}
				}
			}

			if (packet.stream_index == videoStreamIndex)
			{
				avcodec_send_packet(videoCodecContext, &packet);

				av_frame_unref(frame);

				AVFrame* rgbFrame = av_frame_alloc();

				if (avcodec_receive_frame(videoCodecContext, frame) == 0)
				{
					// Just want to get rid of warnings.
					if (frame == nullptr)
					{
						continue;
					}

					if (!rgbFrame)
					{
						std::cout << "ERROR::FFMPEG: rgbFrame not allocated succesfully." << std::endl;
					}

					if (frame->format == AV_PIX_FMT_YUVJ420P)
					{
						AVPixelFormat srcFormat = AV_PIX_FMT_YUV420P;
						AVPixelFormat targetFormat = AV_PIX_FMT_RGB24;
						struct SwsContext* swsContext = sws_getContext(
							frame->width, frame->height, srcFormat,
							frame->width, frame->height, targetFormat,
							0, nullptr, nullptr, nullptr
						);

						if (!swsContext)
						{
							std::cout << "ERROR::FFMPEG: swsContext not created succesfully." << std::endl;
						}

						// Just want to get rid of warnings.
						if (rgbFrame == nullptr)
						{
							continue;
						}

						av_image_alloc(
							rgbFrame->data, rgbFrame->linesize,
							frame->width, frame->height, targetFormat, 1
						);

						sws_scale(
							swsContext, frame->data, frame->linesize,
							0, frame->height, rgbFrame->data, rgbFrame->linesize
						);

						sws_freeContext(swsContext);
					}

					// Just want to get rid of warnings.
					if (rgbFrame == nullptr)
					{
						continue;
					}

					glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
					glClear(GL_COLOR_BUFFER_BIT);

					// Map FFmpeg pixel format to OpenGL format.
					GLenum format = GL_RGB;

					// Bind video frame data as texture.
					glBindTexture(GL_TEXTURE_2D, textureID);
					glTexImage2D(GL_TEXTURE_2D, 0, format, frame->width, frame->height, 0, format, GL_UNSIGNED_BYTE, rgbFrame->data[0]);
					glBindTexture(GL_TEXTURE_2D, 0);

					// Draw.
					glUseProgram(shaderProgram);
					glUniform1i(glGetUniformLocation(shaderProgram, "ourTexture"), 0);
					glBindVertexArray(VAO);
					glActiveTexture(GL_TEXTURE0);
					glBindTexture(GL_TEXTURE_2D, textureID);
					glDrawArrays(GL_TRIANGLES, 0, 6);
					glBindVertexArray(0);
					glBindTexture(GL_TEXTURE_2D, 0);

					av_freep(&rgbFrame->data[0]);
					av_frame_free(&rgbFrame);
				}
			}

			av_packet_unref(&packet);
		}

		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	av_frame_free(&frame);
	avformat_close_input(&formatContext);
	avcodec_free_context(&videoCodecContext);
	avcodec_free_context(&audioCodecContext);

	glfwTerminate();

	return 0;
}