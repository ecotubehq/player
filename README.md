**EcoTubeHQ**



In summary...



EcoTubeHQ Video Player offers high quality video playback for many DRM-free streaming services at minimal bitrates. Hardware acceleration is utilised where possible during video playback to optimize quality and minimize CPU usage.



![screenshot](https://raw.githubusercontent.com/ecotubehq/player/master/ecotube-3.png)

More details...



EcoTubeHQ offers multiple methods of video upscaling - AMD FidelityFX Super Resolution (best quality), Lanczos (good quality) and Bicubic (energy Saving). AMD FSR video quality offers a practical use for 144p video when full screen output is not required, e.g. when listening to podcasts etc.



[Below is the official list of graphics cards that support AMD FSR](https://www.tomshardware.com/reference/amd-fsr-fidelityfx-super-resolution-explained), though other GPUs may work as well:



- AMD Radeon 6000 Series

- AMD Radeon 6000M Series

- AMD Radeon 5000 Series

- AMD Radeon 5000M Series

- AMD Radeon VII

- AMD Radeon RX Vega Series

- AMD Radeon 600 Series

- AMD Radeon RX 500 Series

- AMD Radeon RX 480, 470 and 460 

- AMD Ryzen desktop CPUs with AMD Radeon graphics

- AMD Ryzen mobile CPUs with Radeon Graphics

- Nvidia GeForce RTX 30 Series

- Nvidia GeForce RTX 20 Series

- Nvidia GeForce 16 Series

- Nvidia GeForce 10 Series





EcoTubeHQ can be set to play av1 codec YouTube videos, if av1 isn't available then vp9 video is output, and if vp9 isn't available then the player drops to h.264 output. This allows the player to output the highest quality / lowest bitrate video available. Alternatively the user can set the player to output vp9 or h.264 by default to reduce battery consumption.



EcoTubeHQ restricts video resolution to 720p / 30fps to minimize data usage. There is little (if any) difference when comparing 720p with AMD FSR upscaling to 1080p video quality, but there is a significant reduction in data usage.



Finally, EcoTube offers two different Opus audio bitrates when playing av1 and vp9 YouTube videos - 70kbps (Lo) and 160kbps (Hi).



How to use.



To play a video from an online streaming service simply copy the video URL from the browser, click '+' at the top left of the player and select 'Open Location'.



![screenshot](https://raw.githubusercontent.com/ecotubehq/player/master/ecotube-1.png)



The player automatically places the copied video location imto the 'Location' entry box - click the 'Open' button to initiate playback.



Press the spacebar or right click the mouse on the player screen to pause / play video playback.



EcoTubeHQ Preferences.



AV Options



Video Output



BQ - Best Quality - AMD FSR Video Upscaling [Default] 

HQ - High Guality - Lanczos Video Upscaling 

LE - Low Energy - Bicubic Video Upscaling



YouTube Options



Supported Video Codecs:



av1 - Best quality video with lowest bitrate [Default] 

vp9 - Good quality video with low bitrate 

h.264 - Lowest quality video with the highest bitrate



\*Note: If the video codec is set to av1 but this codec isn't available then vp9 will be utilised, and if vp9 isn't available then h.264 will be used.



Supported Video Resolutions:



720p 

480p [Default] 

360p 

240p 

144p



\*Note: To minimize video data usage 1080p and 60fps are not supported.



Audio Quality *av1 and vp9 only*



Hi - 160Kbps 

Lo - 70Kbps [Default]



Playlists



To create a playlist click on the bottom right icon titled "Toggle Playlist". When this is done the playlist opens.



To add a video from an external streaming service copy the browser URL for the video, then right-click on the playlist, select "Add Location", and click "Open".



Additional videos can be added prior to, and during playback. To start playback simply double-click on the required playlist video.



EcoTubeHQ Original Concept: Colin Bett 

Coding and additional ideas: Sako Adams



Forked from the Celluloid Video Player



This application comes with absolutely no warranty. See the GNU General Public Licence, version 3 or later for details - <https://www.gnu.org/licenses/gpl-3.0.html>.



