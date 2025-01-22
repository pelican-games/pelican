#include <pelican/pelican.hpp>
#include <iostream>
#include <AL/alut.h>
#include <thread>
#include <GLFW/glfw3.h>

bool isPlaying = false; // ���y�̍Đ���Ԃ��Ǘ�����t���O

void toggleAudio(pl::Speaker& speaker) {
    if (isPlaying) {
        // ���y���~
        speaker.stop();
        std::cout << "Audio stopped." << std::endl;
    }
    else {
        // ���y���Đ�
        speaker.play();
        std::cout << "Audio playing." << std::endl;
    }
    isPlaying = !isPlaying;
}

int main() {
    //try {
        //GLFWwindow* window = nullptr;
        //pl::system_init();

        // OpenAL ������
        alutInit(nullptr, nullptr);
        ALuint hellobuffer = alutCreateBufferHelloWorld(), hellosource;
        alGenSources(1, &hellosource);
        alSourcei(hellosource, AL_BUFFER, hellobuffer);
        alSourcePlay(hellosource);
        
        // �I�[�f�B�I�t�@�C�������[�h
        pl::Audio audio;
        std::cout << "Which Audio do you want?" << std::endl;
        std::string audio_name;
        std::cin >> audio_name;
        std::filesystem::path path = std::filesystem::current_path() / audio_name;
        ALuint test_buf, test_src;
        test_buf = alutCreateBufferFromFile(path.string().c_str());
        if (test_buf == AL_NONE) {
            ALenum error = alutGetError();
            std::cerr << "Failed to create buffer: " << alutGetErrorString(error) << std::endl;
        }
        alGenSources(1, &test_src);
        alSourcei(test_src, AL_BUFFER, test_buf);
        alSourcePlay(test_src);
        /*try {
            audio.loadAudio(path);
            long long tmp = 0;
            for (long long i = 0; i < 1e10; i++) {
                tmp += i;
                std::cout << "You can do it!" << std::endl;
            }
        }
        catch (const std::exception & e){
            std::cerr << "Audio load failed: " << e.what() << std::endl;
            long long tmp = 0;
            for (long long i = 0; i < 1e10; i++) {
                tmp += i;
            }
        }*/

        /*
        // �X�s�[�J�[��ݒ�
        pl::Speaker speaker;
        speaker.set(audio);
        speaker.setVolume(1.0f); // ���ʂ��ő�ɐݒ�
        speaker.setLoop(true);   // ���[�v�Đ���L����
        // �V�X�e���������iGLFW �E�B���h�E�쐬�Ȃǁj
        // ���C�����[�v
        std::cout << "Press SPACE to toggle audio playback. Press ESC to exit." << std::endl;
        while (pl::frame_update()) {
            // �X�y�[�X�L�[�ŉ��y�̍Đ�/��~��؂�ւ�
            if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) {
                toggleAudio(speaker);

                // �L�[�̉�����1�񕪂�������
                while (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(100));
                }
            }

            // ESC�L�[�ŃE�B���h�E�����
            if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
                break;
            }
        }

        // �N���[���A�b�v
        pl::cleanup();
        alutExit();
        std::cout << "Application terminated gracefully." << std::endl;
    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return -1;
    }
    */
        alutSleep(1);
        alutExit();
    return 0;
}
