# Linux support for EgisTec EH0576

After installing Linux on my [IdeaPad Flex 5 14ALC7 Laptop](https://pcsupport.lenovo.com/de/en/products/laptops-and-netbooks/flex-series/flex-5-14alc7/) the fingerprint reader did not work anymore because there was no driver for the **EH0576** (`1c7a:0576`) fingerprint reader.

## Thank You

Huge credit goes to [Animeshz](https://github.com/Animeshz) as they laid the base for this effort in a [merge request](https://gitlab.freedesktop.org/libfprint/libfprint/-/merge_requests/317) of [`libfprint`](https://gitlab.freedesktop.org/libfprint/libfprint).
Initial tests proved that the **EH0575** and the **EH0576** (likely the successor) seem to work basically the same as by just providing the **EH0575** packets I was able to occasionally get little data out of mine.

## Work

I was unable to get the fingerprint reader to work in a VM (both Win10 and Win11) and Wireshark USBcap does not allow to be run on Windows-To-Go.

Therefore, the rest of the reverse engineering was solely done with static analysis in **Ghidra** of the `EgisTouchFP0576.dll` UMDF driver provided by [Lenovo](https://pcsupport.lenovo.com/de/en/products/laptops-and-netbooks/flex-series/flex-5-14alc7/downloads/driver-list/component?name=Fingerprint%20Reader&id=A4F7592E-3C73-4CCF-ABB9-09549219DFC8). I am no professional "hacker" by any means so I cannot guarantee that this project is 100% correct.

## Contributing

If you can help me improve this project feel free to open a pull request.
