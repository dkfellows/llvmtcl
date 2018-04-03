Make sure your PR is against the right repository!
==================================================
The baseline for pull requests should (normally) be `dkfellows:master`.

Describe what your Pull Request is implementing
-----------------------------------------------
Is it fixing an issue? Is it adding a new capability? I'd love to know what you're trying to do so that I can independently check whether it is achieving it.


Describe the platform you are using
-----------------------------------
llvmtcl is very dependent on the platform it is built and running on. Please describe what you have developed your PR upon reasonably accurately so that I can try to ensure that your code is likely to work on other platforms as well. (There are a few tricky gotchas, alas.)

llvmtcl is also dependent on the versions of Tcl and LLVM that you are using. Please describe them. (If you're on MacOS, remember to also describe how you acquired Tcl and LLVM. Are you using the system versions, have you installed them with a third-party package manager such as MacPorts or HomeBrew, or have you built your own versions directly from source? _This is known to be able to affect the build._)
