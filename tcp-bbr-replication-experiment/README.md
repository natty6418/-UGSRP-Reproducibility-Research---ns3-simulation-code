To run this experiment on FABRIC - 

### Get resources on FABRIC

In a Python notebook in the FABRIC Jupyter environment,

1. Configure your FABRIC account

```python
from fabrictestbed_extensions.fablib.fablib import FablibManager as fablib_manager
fablib = fablib_manager() 
conf = fablib.show_config()
```

2. Get a random site, make sure it has cores available

```python
site_name = fablib.get_random_site()
fablib.show_site(site_name)
```

3. Reserve a VM with lots of cores and memory

```python
slice = fablib.new_slice(name='hpc')
slice.add_node(name='hpc', site=site_name, 
                   cores=64, 
                   ram=256, 
                   disk=100, 
                   image='default_ubuntu_22')
slice.submit()
```

Log in the resource over SSH once it is ready.

### Install ns3 on the resource

<!-- TODO: fill in the details -->

```
sudo apt install g++ python3 python3-pip cmake ninja-build git ccache -y
wget https://www.nsnam.org/releases/ns-allinone-3.42.tar.bz2
tar jxf ns-allinone-3.42.tar.bz2
cd ns-allinone-3.42/ 
```

### Install SLURM on the resource

```
sudo apt update -y
sudo apt install slurmd slurmctld -y
```

```
sudo mkdir /etc/slurm-llnl/
sudo chmod 777 /etc/slurm-llnl
sudo mkdir /var/lib/slurm-llnl/
sudo mkdir /var/log/slurm-llnl/
sudo chmod 777 /var/lib/slurm-llnl/
sudo chmod 777 /var/log/slurm-llnl/
```
