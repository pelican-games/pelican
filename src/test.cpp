#include <pelican/pelican.hpp>
#include <iostream>
#include <AL/alut.h>
#include <thread>
#include <GLFW/glfw3.h>

bool isPlaying = false; // 音楽の再生状態を管理するフラグ

void toggleAudio(pl::Speaker& speaker) {
    if (isPlaying) {
        // 音楽を停止
        speaker.stop();
        std::cout << "Audio stopped." << std::endl;
    }
    else {
        // 音楽を再生
        speaker.play();
        std::cout << "Audio playing." << std::endl;
    }
    isPlaying = !isPlaying;
}

int main() {
    try {
        GLFWwindow* window = nullptr;
        pl::system_init();

        // OpenAL 初期化
        alutInit(nullptr, nullptr);

        // オーディオファイルをロード
        pl::Audio audio;
        std::filesystem::path audio_name;
        std::cout << "Which Audio do you want?" << std::endl;
        std::cin >> audio_name;
        audio.loadAudio(audio_name);

        // スピーカーを設定
        pl::Speaker speaker;
        speaker.set(audio);
        speaker.setVolume(1.0f); // 音量を最大に設定
        speaker.setLoop(true);   // ループ再生を有効化

        // システム初期化（GLFW ウィンドウ作成など）
        pl::system_init();

        // メインループ
        std::cout << "Press SPACE to toggle audio playback. Press ESC to exit." << std::endl;
        while (pl::frame_update()) {
            // スペースキーで音楽の再生/停止を切り替え
            if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) {
                toggleAudio(speaker);

                // キーの押下を1回分だけ処理
                while (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(100));
                }
            }

            // ESCキーでウィンドウを閉じる
            if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
                break;
            }
        }

        // クリーンアップ
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
