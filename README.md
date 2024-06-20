# CMoe-Counter

多种风格可选的萌萌计数器的简易C语言版本

<div align=center> <a href="#"> <img src="https://counter.seku.su/cmoe?name=cmoe&theme=gb" /> </a> </div>

<details>
<summary>More theme</summary>

##### moebooru(mb)
![moebooru](https://counter.seku.su/cmoe?name=demo&theme=mb)

##### rule34(r34)
![Rule34](https://counter.seku.su/cmoe?name=demo&theme=r34)

##### gelbooru(gb)
![Gelbooru](https://counter.seku.su/cmoe?name=demo&theme=gb)

##### nixie(nix)
![Nixie Tube](https://counter.seku.su/cmoe?name=demo&theme=nix)

##### mobius(mbs)
![Nixie Tube](https://counter.seku.su/cmoe?name=demo&theme=mbs)

</details>

## Usage
### 1. Install [simple-http-server](https://github.com/fumiama/simple-http-server)
Follow the instructions in that repo to install it.
### 2. Install [simple-protobuf](https://github.com/fumiama/simple-protobuf)
Follow the instructions in that repo to install it.
### 3. Install CMoe
```bash
git clone https://github.com/fumiama/CMoe-Counter.git
cd CMoe-Counter
mkdir build
cd build
cmake ..
make
# make install
```
### 4. Copy files to web root
1. Copy `favicon.ico`, `index.html`, `style.css` and the compiled binary `cmoe` (in the `build` folder) into the same folder.
2. Run `simple-http-server` by the command
```bash
simple-http-server -d port folder
```
### 5. Enjoy it!
You can change token and datafile by passing env `TOKEN` & `DATFILE`.

## API
The API of CMoe is simpler than [Moe-counter](https://github.com/journey-ad/Moe-counter)
### 1. Register
```
http://yourdomain/cmoe?name=yourname&reg=token
```
### 2. Refer
```
http://yourdomain/cmoe?name=yourname(&theme=mb/mbh/gb/gbh/r34/nix/mbs)
```
That's all.
## Credits
*   [Moe-counter](https://github.com/journey-ad/Moe-counter)
*   [moebooru](https://github.com/moebooru/moebooru)
*   rule34.xxx NSFW
*   gelbooru.com NSFW
*   [Icons8](https://icons8.com/icons/set/star)
