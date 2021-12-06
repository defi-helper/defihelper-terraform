resource "yandex_compute_instance" "bastion" {
  name     = "${var.name}-bastion"
  hostname = "bastion"
  zone     = var.zone
  allow_stopping_for_update = var.bastion_allow_stopping_for_update

  boot_disk {
    auto_delete = true
    initialize_params {
      size     = 15
      type     = "network-hdd"
      image_id = var.bastion_vm_image
    }
  }

  resources {
    core_fraction = var.bastion_core_fractions
    cores         = var.bastion_cores
    memory        = var.bastion_memory
  }

  scheduling_policy { preemptible = false }
  network_interface {
    subnet_id      = var.subnet_id
    ip_address     = var.ip_address
    nat_ip_address = var.bastion_nat_ip_address
    nat            = true
  }

  metadata = {
    user-data = file(var.bastion_ssh_users_file_path)
  }
}
