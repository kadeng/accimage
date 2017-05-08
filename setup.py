from distutils.core import setup, Extension

accimage = Extension('accimage',
                    include_dirs = ['/usr/local/opt/jpeg-turbo/include', '/opt/intel/ipp/include'],
                    libraries = ['jpeg', 'ippi', 'ipps'],
                    library_dirs = ['/usr/local/opt/jpeg-turbo/lib', '/opt/intel/ipp/lib', '/opt/intel/compilers_and_libraries_2017.2.174/linux/ipp/lib/intel64_lin/'],
                    sources = ['accimagemodule.c', 'jpegloader.c', 'imageops.c'])

setup(name = 'accimage',
      version = '0.1',
      description = 'Accelerated image loader and preprocessor for Torch',
      author = 'Marat Dukhan',
      author_email = 'maratek@gmail.com',
      ext_modules = [accimage])
