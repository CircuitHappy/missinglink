# -*- mode: ruby -*-
# vi: set ft=ruby :
def which(cmd)
  exts = ENV['PATHEXT'] ? ENV['PATHEXT'].split(';') : ['']
  ENV['PATH'].split(File::PATH_SEPARATOR).each do |path|
    exts.each { |ext|
      exe = File.join(path, "#{cmd}#{ext}")
      return exe if File.executable?(exe) && !File.directory?(exe)
    }
  end
  return nil
end

# usbfilter_exists and better_usbfilter_add originally part of a pull request 
# https://github.com/mitchellh/vagrant/issues/5774
def usbfilter_exists(vendor_id, product_id)
    # Determine if a usbfilter with the provided Vendor/Product ID combination
    # already exists on this VM.
    # NOTE: The "machinereadable" output for usbfilters is more
    #       complicated to work with (due to variable names including
    #       the numeric filter index) so we don't use it here.
    #
    machine_id_filepath = File.join(".vagrant", "machines", "default", "virtualbox", "id")

    if not File.exists? machine_id_filepath
      # VM hasn't been created yet.
      return false
    end

    machine_id = File.read(machine_id_filepath)

    vm_info = `VBoxManage showvminfo #{machine_id}`
    filter_match = "VendorId:         #{vendor_id}\nProductId:        #{product_id}\n"
    
    return vm_info.include? filter_match
end

def better_usbfilter_add(vb, vendor_id, product_id, filter_name)
    # This is a workaround for the fact VirtualBox doesn't provide
    # a way for preventing duplicate USB filters from being added.
    #
    # TODO: Implement this in a way that it doesn't get run multiple
    #       times on each Vagrantfile parsing.
    if not usbfilter_exists(vendor_id, product_id)
      vb.customize ["usbfilter", "add", "0",
                    "--target", :id,
                    "--name", filter_name,
                    "--vendorid", vendor_id,
                    "--productid", product_id
                    ]
    end
end

# All Vagrant configuration is done below. The "2" in Vagrant.configure
# configures the configuration version (we support older styles for
# backwards compatibility). Please don't change it unless you know what
# you're doing.
Vagrant.configure(2) do |config|
  # The most common configuration options are documented and commented below.
  # For a complete reference, please see the online documentation at
  # https://docs.vagrantup.com.

  # Every Vagrant development environment requires a box. You can search for
  # boxes at https://atlas.hashicorp.com/search.
  config.vm.box = "ubuntu/xenial32"

  # Disable automatic box update checking. If you disable this, then
  # boxes will only be checked for updates when the user runs
  # `vagrant box outdated`. This is not recommended.
  # config.vm.box_check_update = false

  # Create a forwarded port mapping which allows access to a specific port
  # within the machine from a port on the host machine. In the example below,
  # accessing "localhost:8080" will access port 80 on the guest machine.
  # config.vm.network "forwarded_port", guest: 80, host: 8080

  # Create a private network, which allows host-only access to the machine
  # using a specific IP.
  # config.vm.network "private_network", ip: "192.168.33.10"

  # Create a public network, which generally matched to bridged network.
  # Bridged networks make the machine appear as another physical device on
  # your network.
  # config.vm.network "public_network"

  # Share an additional folder to the guest VM. The first argument is
  # the path on the host to the actual folder. The second argument is
  # the path on the guest to mount the folder. And the optional third
  # argument is a set of non-required options.
  config.vm.synced_folder ".", "/home/ubuntu/CHIP-SDK"

  # Provider-specific configuration so you can fine-tune various
  # backing providers for Vagrant. These expose provider-specific options.
  # Example for VirtualBox:
  #
  config.vm.provider "virtualbox" do |vb|
  #   # Display the VirtualBox GUI when booting the machine
  #   vb.gui = true
  #
     # Customize the amount of memory on the VM:
     # Git fails to clone the
     vb.memory = "1024"
     vb.customize ['modifyvm', :id, '--usb', 'on']
     vb.customize ['modifyvm', :id, '--usbehci', 'on']
     unless which('VBoxManage').nil?
         better_usbfilter_add(vb, "18d1", "1010", "CHIP in fastboot mode")
         better_usbfilter_add(vb, "1f3a", "1010", "CHIP in fastboot mode")
         better_usbfilter_add(vb, "0525", "a4a7", "CHIP Linux Gadget USB Serial Port")
         better_usbfilter_add(vb, "067b", "2303", "PL2303 Serial Port")
         better_usbfilter_add(vb, "1f3a", "efe8", "CHIP")
         
     end
  end
  #
  # View the documentation for the provider you are using for more
  # information on available options.

  # Define a Vagrant Push strategy for pushing to Atlas. Other push strategies
  # such as FTP and Heroku are also available. See the documentation at
  # https://docs.vagrantup.com/v2/push/atlas.html for more information.
  # config.push.define "atlas" do |push|
  #   push.app = "YOUR_ATLAS_USERNAME/YOUR_APPLICATION_NAME"
  # end

  # Enable provisioning with a shell script. Additional provisioners such as
  # Puppet, Chef, Ansible, Salt, and Docker are also available. Please see the
  # documentation for more information about their specific syntax and use.
  config.vm.provision "shell", path:"./setup_ubuntu1604.sh"
end
