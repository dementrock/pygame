try:
    import pygame2.test.pgunittest as unittest
except:
    import pgunittest as unittest

import pygame2
import pygame2.sdl.base as base
import pygame2.sdl.constants as constants

class SDLTest (unittest.TestCase):

    def test_pygame2_sdl_base_get_compiled_version(self):

        # __doc__ (as of 2009-04-01) for pygame2.sdl.base.get_compiled_version:

        # Gets the SDL version pygame2 was compiled against as three-value
        # tuple.  This version is built at compile time. It can be used to
        # detect which features may not be available through Pygame, if it is
        # used as precompiled package using a different version of the SDL
        # library.
        # 
        # Gets the SDL version pygame2 was compiled against as three-value
        # tuple.  This version is built at compile time. It can be used to
        # detect which features may not be available through Pygame, if it is
        # used as precompiled package using a different version of the SDL
        # library.
        self.assertEquals (len (base.get_compiled_version ()), 3)
        self.assertEquals (base.get_compiled_version ()[0], 1)
        self.assertEquals (base.get_compiled_version ()[1], 2)
        self.assert_ (base.get_compiled_version ()[2] >= 10)

    def test_pygame2_sdl_base_get_error(self):

        # __doc__ (as of 2009-04-01) for pygame2.sdl.base.get_error:

        # Gets the last :exc:pygame2.base.Error occured.
        # 
        # Gets the last :exc:pygame2.base.Error occured.  SDL maintains an
        # internal error message. This message will usually be given to you
        # when a :exc:pygame2.base.Error is raised. You will rarely need to
        # call this function.
        self.assertEquals (len (base.get_error ()), 0)

    def test_pygame2_sdl_base_get_version(self):

        # __doc__ (as of 2009-04-01) for pygame2.sdl.base.get_version:

        # Gets the SDL version pygame2 currently uses as three-value tuple.
        # 
        # Gets the SDL version pygame2 currently uses as three-value tuple.
        # This version is detected at runtime. It can be used to detect which
        # features may not be available through Pygame, if it is used as
        # precompiled package using a different version of the SDL library.
        self.assertEquals (len (base.get_version ()), 3)
        self.assertEquals (base.get_version ()[0], 1)
        self.assertEquals (base.get_version ()[1], 2)

    def test_pygame2_sdl_base_init(self):

        # __doc__ (as of 2009-04-01) for pygame2.sdl.base.init:

        # Initializes the underlying SDL library.
        # 
        # Initializes the underlying SDL library.  Initializes the underlying
        # SDL library using the passed SDL flags. The flags indicate, which
        # subsystems of SDL should be initialized and can be a bitwise
        # combination of the INIT_* constants.  In case an error occured,
        # False will be returned. The detailled error can be received using
        # pygame2.sdl.get_error().
        # 
        # Initializes the underlying SDL library.  Initializes the underlying
        # SDL library using the passed SDL flags. The flags indicate, which
        # subsystems of SDL should be initialized and can be a bitwise
        # combination of the INIT_* constants.  In case an error occured,
        # False will be returned. The detailled error can be received using
        # pygame2.sdl.get_error().
        self.assertEquals (base.init (constants.INIT_VIDEO), True)
        self.assertEquals (base.init
                           (constants.INIT_VIDEO | constants.INIT_AUDIO), True)

    def test_pygame2_sdl_base_init_subsystem(self):

        # __doc__ (as of 2009-04-01) for pygame2.sdl.base.init_subsystem:

        # Initializes one or more SDL subsystems.
        # 
        # Initializes one or more SDL subsystems.  In case a specific part of
        # SDL was not initialized using pygame2.sdl.init(), this funciton can
        # be used to initialize it at a later time.  In case an error occured,
        # False will be returned. The detailled error can be received using
        # pygame2.sdl.get_error().
        # 
        # Initializes one or more SDL subsystems.  In case a specific part of
        # SDL was not initialized using pygame2.sdl.init(), this funciton can
        # be used to initialize it at a later time.  In case an error occured,
        # False will be returned. The detailled error can be received using
        # pygame2.sdl.get_error().
        self.assertEquals (base.init_subsystem (constants.INIT_CDROM), True)
        self.assertEquals (base.init_subsystem
                           (constants.INIT_CDROM | constants.INIT_TIMER), True)

    def test_pygame2_sdl_base_quit(self):

        # __doc__ (as of 2009-04-01) for pygame2.sdl.base.quit:

        # Shuts down all subsystems of the underlying SDL library.
        # 
        # Shuts down all subsystems of the underlying SDL library.  After
        # calling this function, you should not invoke any SDL related class,
        # method or function as they are likely to fail or might give
        # unpredictable results.
        self.assert_ (base.quit () == None)

    def test_pygame2_sdl_base_quit_subsystem(self):

        # __doc__ (as of 2009-04-01) for pygame2.sdl.base.quit_subsystem:

        # Shuts down one or more subsystems of the underlying SDL library.
        # 
        # Shuts down one or more subsystems of the underlying SDL library.
        # After calling this function, you should not invoke any class, method
        # or function related to the specified subsystems as they are likely
        # to fail or might give unpredictable results.
        self.assert_ (base.quit_subsystem (constants.INIT_AUDIO) == None)
        self.assert_ (base.quit_subsystem (constants.INIT_CDROM) == None)

    def test_pygame2_sdl_base_was_init(self):

        # __doc__ (as of 2009-04-01) for pygame2.sdl.base.was_init:

        # Gets a bitwise OR'ed combination of the initialized SDL subsystems.
        # 
        # Gets a bitwise OR'ed combination of the initialized SDL subsystems.
        # Returns a bitwise combination of the currently initialized SDL
        # subsystems.
        base.init (constants.INIT_VIDEO)
        v = base.was_init (constants.INIT_VIDEO) & constants.INIT_VIDEO
        self.assert_ (v == constants.INIT_VIDEO)
