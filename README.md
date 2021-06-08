# Moe-counter

多种风格可选的萌萌计数器

![Moe-counter](http://pan.fumiama.top:42412/cmoe?name=cmoe)

<details>
<summary>More theme</summary>

##### moebooru(mb)
![moebooru](https://count.getloli.com/get/@demo?theme=moebooru)

##### rule34(r34)
![Rule34](https://count.getloli.com/get/@demo?theme=rule34)

##### gelbooru(gb)
![Gelbooru](https://count.getloli.com/get/@demo?theme=gelbooru)</details>

## Demo
[https://count.getloli.com](https://count.getloli.com)

## Usage
### Install [simple-http-server](https://github.com/fumiama/simple-http-server)
Follow the instructions in [this repo](https://github.com/fumiama/simple-http-server) to install it.
### Install CMoe
```bash
git clone https://github.com/fumiama/CMoe-Counter.git
cd CMoe-Counter
mkdir build
cd build
cmake ..
make
make install
```
### Copy files
1. Copy `favicon.ico`, `index.html`, `style.css` and the compiled binary `cmoe` into the same folder.
2. Run `simple-http-server` by the command
```bash
simple-http-server -d port folder
```
### Enjoy it!

## API
The API of CMoe is simpler than [Moe-counter](https://github.com/journey-ad/Moe-counter)
### Register
`token` is defined in `cmoe.h`
```
http://yourdomain/cmoe?name=yourname&reg=token
```
### Refer
```
http://yourdomain/cmoe?name=yourname(&theme=mb/mbh/gb/gbh/r34)
```
That's all.
## Credits
*   [Moe-counter](https://github.com/journey-ad/Moe-counter)
*   [moebooru](https://github.com/moebooru/moebooru)
*   rule34.xxx NSFW
*   gelbooru.com NSFW
*   [Icons8](https://icons8.com/icons/set/star)
