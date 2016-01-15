from ert.geo import Surface
from ert.test import ExtendedTestCase , TestAreaContext


class SurfaceTest(ExtendedTestCase):
    def setUp(self):
        self.surface_valid = self.createTestPath("local/geometry/surface/valid_ascii.irap")
        self.surface_short = self.createTestPath("local/geometry/surface/short_ascii.irap")
        self.surface_long  = self.createTestPath("local/geometry/surface/long_ascii.irap")


        
    def test_create(self):
        with self.assertRaises(IOError):
            s = Surface("File/does/not/exist")

        with self.assertRaises(ValueError):
            s = Surface(self.surface_short)

        with self.assertRaises(ValueError):
            s = Surface(self.surface_long)

        s = Surface( self.surface_valid )

        self.assertEqual( s.getNX( ) , 49 )
        self.assertEqual( s.getNY( ) , 79 )
        self.assertEqual( len(s) , 49*79 )

        with self.assertRaises(IndexError):
            v = s[49 * 79]

        with self.assertRaises(TypeError):
            v = s["KEY"]

        self.assertEqual( s[0]  ,  0.0051 )
        self.assertEqual( s[-1] , -0.0014 )

        with self.assertRaises(IndexError):
            s[49*79] = 787
        
        s[0] = 10
        self.assertEqual( s[0]  ,  10 )

        s[-1] = 77
        self.assertEqual( s[len(s) - 1]  ,  77 )

        
    def test_write(self):
        with TestAreaContext("surface/write"):
            
            s0 = Surface( self.surface_valid )
            s0.write( "new_surface.irap")

            s1 = Surface( "new_surface.irap")
            self.assertTrue( s1 == s0 )

            s0[0] = 99
            self.assertFalse( s1 == s0 )



    def test_copy(self):
        with TestAreaContext("surface/copy"):
            s0 = Surface( self.surface_valid )
            s1 = s0.copy( )

            self.assertTrue( s1 == s0 )
            s1[0] = 99
            self.assertFalse( s1 == s0 )
            del s0
            self.assertEqual( s1[0] , 99)
            
            s2 = s1.copy( copy_data = False )
            self.assertEqual( s2[0] , 0.0 )
            self.assertEqual( s2[10] , 0.0 )
            self.assertEqual( s2[100] , 0.0 )
            
