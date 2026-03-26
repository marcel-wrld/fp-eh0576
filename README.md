# Linux support for EgisTec EH0576

After installing Linux on my [IdeaPad Flex 5 14ALC7 Laptop](https://pcsupport.lenovo.com/de/en/products/laptops-and-netbooks/flex-series/flex-5-14alc7/) the fingerprint reader did not work anymore because there was no driver for the **EH0576** (`1c7a:0576`) fingerprint reader.

## WIP State
Currently the driver still has a fair amount of false rejection rates but it works a "fair" (well..depends on your expectations) amount of time.

- `ghidra/`:
Contains the **Ghidra** project files where I renamed some functions and variables of the `EgisTouchFP0576.dll` as good as I could.
You can get the `.dll` driver files from the Lenovo download page down below.
- `libfprint/`:
Contains the driver files for integration into `libfprint`. I have yet to create a patch for it.
- `usb_research/`:
Contains the **CMake** project I used for reverse engineering and testing efforts before I went on to write the `libfprint` driver.

## Thank You

Huge credit goes to [Animeshz](https://github.com/Animeshz)'s efforts [on github](https://github.com/Animeshz/EgisTec-EH575) and [on `libfprint`](https://gitlab.freedesktop.org/libfprint/libfprint/-/merge_requests/317) as those laid the base for this effort.
Initial tests proved that the **EH0575** and the **EH0576** seem to work similar as by just providing the **EH0575** packets I was able to occasionally get little data out of mine even though the **EH0576** is not a swipe sensor.

## Reverse Engineering

I was unable to get the fingerprint reader to work in a VM (both Win10 and Win11) and Wireshark USBcap does not allow to be run on Windows-To-Go so I could not use any USB captures from Windows.

Therefore, the rest of the reverse engineering was solely done with static analysis in **Ghidra** of the `EgisTouchFP0576.dll` UMDF driver provided by [Lenovo](https://pcsupport.lenovo.com/de/en/products/laptops-and-netbooks/flex-series/flex-5-14alc7/downloads/driver-list/component?name=Fingerprint%20Reader&id=A4F7592E-3C73-4CCF-ABB9-09549219DFC8). I am no professional "hacker" by any means so I cannot guarantee that this project is 100% correct.

## Functionality

As you might already know `libfprint` is pretty bad at processing small sensors: the **EH0576** has an output image of `70x57`.
Therefore I implemented three post-processing steps:
1. Normalize the image.
2. Upscale it by a factor of 2 to `140x114`.
3. Put into the center of a `256x256` white canvas.

This seems to be okay enough for `libfprint` to generate scores and even matches.

The biggest **issue** beside unreliable matching scores is that the `BZ3` threshold has to be reduced to around `10` which is a pretty low threshold and therefore imposes a security risk. While I managed to reach pretty high scores they don't happen often enough to increase it to a higher value (for now?) .

_(To be fair in order to reduce the pain of using the driver in it's current state it would likely require me to lower the value even more but that would be VERY bad. I do not recommend doing this.)_ 

## Contributing

If you can help me improve this project feel free to open a pull request.

