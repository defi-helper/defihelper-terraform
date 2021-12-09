resource "yandex_mdb_redis_cluster" "redis_regional" {
  name        = var.redis_name
  environment = "PRESTABLE"
  sharded     = true
  network_id  = var.vpc_id

  config {
    password = local.password
    version  = "6.0"
  }

  resources {
    resource_preset_id = "b2.nano"
    disk_size          = 16
  }

  host {
    zone      = var.location_subnets[0].zone
    subnet_id = var.location_subnets[0].id
    shard_name = "first"
  }

  host {
    zone      = var.location_subnets[1].zone
    subnet_id = var.location_subnets[1].id
    shard_name = "second"
  }

  host {
    zone      = var.location_subnets[2].zone
    subnet_id = var.location_subnets[2].id
    shard_name = "third"
  }
}

resource "random_string" "redis-password" {
  length  = 16
  special = false
}

locals {
  password = random_string.redis-password.result
}
