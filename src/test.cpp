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
    try {
        GLFWwindow* window = nullptr;
        pl::system_init();

        // OpenAL ������
        alutInit(nullptr, nullptr);

        // �I�[�f�B�I�t�@�C�������[�h
        pl::Audio audio;
        std::filesystem::path audio_name;
        std::cout << "Which Audio do you want?" << std::endl;
        std::cin >> audio_name;
        audio.loadAudio(audio_name);

        // �X�s�[�J�[��ݒ�
        pl::Speaker speaker;
        speaker.set(audio);
        speaker.setVolume(1.0f); // ���ʂ��ő�ɐݒ�
        speaker.setLoop(true);   // ���[�v�Đ���L����

        // �V�X�e���������iGLFW �E�B���h�E�쐬�Ȃǁj
        pl::system_init();

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

    return 0;
}
