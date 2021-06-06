from setuptools import setup, find_packages

setup(
    name='pypomcpp',
    version='1.0.0',
    description='Provides an interface between pomcpp and the python environment',
    packages=find_packages(),    
    install_requires=['pommerman', 'numpy'],
)