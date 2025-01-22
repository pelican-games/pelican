#ifndef PELICAN_SYSTEM_HPP
#define PELICAN_SYSTEM_HPP

#include <vector>
#include <btBulletDynamicsCommon.h>
#include <btBulletCollisionCommon.h>
namespace pl {

    class System {
    public:
        //ゲームそのものに関するもの
        void startGame();
        void pauseGame();
        void restartGame();
        void endGame();
        //ステージなどに関するもの
        void loadStage(std::string stageinfopath);
        void restartStage(std::string stageinfopath);
        //セーブ、ロード
        void saveGamestate(std::string savepath);
        void loadGamestate(std::string loadpath);


        static System& instance();
        ~System();
        
    private:
        System(); 
        bool running = false;
    };

} // namespace pl

#endif // SYSTEM_HPP
