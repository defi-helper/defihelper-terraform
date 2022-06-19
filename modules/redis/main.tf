resource "yandex_mdb_redis_cluster" "redis_regional" {
  name        = var.redis_name
  environment = "PRESTABLE"
  network_id  = var.vpc_id

  timeouts {
    create = "60m"
  }

  config {
    password = local.password
    version  = "6.2"
  }

  resources {
    resource_preset_id = var.redis_host_class
    disk_size          = 16
  }

  host {
    zone      = var.location_subnets[0].zone
    subnet_id = var.location_subnets[0].id
  }
}

resource "random_string" "redis-password" {
  length  = 16
  special = false
}

locals {
  password = random_string.redis-password.result
}
