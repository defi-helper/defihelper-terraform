resource "yandex_compute_instance" "hosting" {
  count      = var.enable_hosting ? 1 : 0
  name     = "${var.name}-hosting"
  hostname = "hosting"
  zone     = var.zone
  allow_stopping_for_update = true
  platform_id = "standard-v3"

  boot_disk {
    auto_delete = true
    initialize_params {
      size     = var.hosting_disk_size
      type     = "network-ssd"
      image_id = var.hosting_vm_image
    }
  }

  resources {
    core_fraction = var.hosting_core_fractions
    cores         = var.hosting_cores
    memory        = var.hosting_memory
  }

  scheduling_policy { preemptible = false }
  network_interface {
    subnet_id      = var.subnet_id
    nat_ip_address = var.hosting_nat_ip_address
    nat            = true
  }

  metadata = {
    user-data = file(var.hosting_ssh_users_file_path)
  }
}
