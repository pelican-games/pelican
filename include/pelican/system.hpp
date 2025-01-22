#ifndef PELICAN_SYSTEM_HPP
#define PELICAN_SYSTEM_HPP

#include <vector>
#include <btBulletDynamicsCommon.h>
#include <btBulletCollisionCommon.h>
namespace pl {

    //�Q�[�����̂��̂Ɋւ������
    void startGame();
    void pauseGame();
    void restartGame();
    void endGame();
    //�X�e�[�W�ȂǂɊւ������
    void loadStage(std::string stageinfopath);
    void restartStage(std::string stageinfopath);
    //�Z�[�u�A���[�h
    void saveGamestate(std::string savepath);
    void loadGamestate(std::string loadpath);

} // namespace pl

#endif // SYSTEM_HPP
