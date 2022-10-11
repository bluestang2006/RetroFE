/* This file is part of RetroFE.
 *
 * RetroFE is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * RetroFE is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with RetroFE.  If not, see <http://www.gnu.org/licenses/>.
 */
#pragma once

#include "IVideo.h"

extern "C"
{
#include <vlc/vlc.h>
}

struct libvlc_instance_t;
struct libvlc_media_t;
struct libvlc_media_player_t;

class VLCVideo : public IVideo
{
public:
    VLCVideo( int monitor );
    ~VLCVideo();
    void setNumLoops(int n);
    SDL_Texture *getTexture() const;
    bool initialize();
    bool deInitialize();
    bool stop();
    bool play(std::string file);
    //void freeElements();
    int getHeight();
    int getWidth();
    void draw();
    void update(float dt);
    bool isPlaying();
    void setVolume(float volume);
    void skipForward( );
    void skipBackward( );
    void skipForwardp( );
    void skipBackwardp( );
    void pause( );
    void restart( );
    unsigned long long getCurrent( );
    unsigned long long getDuration( );
    bool isPaused( );

private:
    static libvlc_instance_t*    VLC;
    libvlc_media_t*              Media;
    libvlc_media_player_t*       MediaPlayer;
    SDL_Texture* texture_;
    int height_;
    int width_;
    bool frameReady_;
    bool isPlaying_;
    static bool initialized_;
    int playCount_;
    std::string currentFile_;
    int numLoops_;
    float volume_;
    double currentVolume_;
    int monitor_;
    bool paused_;
};
