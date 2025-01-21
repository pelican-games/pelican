#ifndef PELICAN_SYSTEM_HPP
#define PELICAN_SYSTEM_HPP

#include <vector>
#include <btBulletDynamicsCommon.h>
#include <btBulletCollisionCommon.h>
namespace pl {

    class System {
    public:
        void startGame();
        void stopGame();
        void resetGame();
        void respawnBall();

        static System& instance();
        ~System();
        
    private:
        System(); 
        bool running = false;
    };

} // namespace pl

#endif // SYSTEM_HPP
