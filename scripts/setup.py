from distutils.core import setup, Extension
module1 = Extension('_skf', source=['skf_convert_f_wrap.c'])
setup (name='skf',
	version='2.0',
	description='simple kanji filter',
	ext_modules = [module1])
