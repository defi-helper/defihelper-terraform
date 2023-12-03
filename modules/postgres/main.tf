resource "yandex_mdb_postgresql_database" "open" {
  cluster_id = yandex_mdb_postgresql_cluster.postgresql_open.id
  name       = "open"
  owner      = var.pg_open_user_name
  lc_collate = "en_US.UTF-8"
  lc_type    = "en_US.UTF-8"
}

resource "yandex_mdb_postgresql_user" "open" {
  cluster_id = yandex_mdb_postgresql_cluster.postgresql_open.id
  name       = var.pg_open_user_name
  password   = var.pg_open_user_password
  conn_limit = var.pg_open_user_conn_limit
}

resource "yandex_mdb_postgresql_cluster" "postgresql_open" {
  name        = "${var.name}-cluster-open"
  folder_id   = var.folder_id
  environment = "PRODUCTION"
  network_id  = var.vpc_id

  config {
    version = var.pg_version

    pooler_config {
      pool_discard = false
      pooling_mode = "SESSION"
    }

    resources {
      resource_preset_id = var.pg_tier_id
      disk_type_id       = "network-ssd"
      disk_size          = var.pg_instance_disk_size
    }
  }

  dynamic "host" {
    for_each = [{
      zone      = var.location_subnets[0].zone
      subnet_id = var.location_subnets[0].id
    }]

    content {
      zone             = host.value.zone
      subnet_id        = host.value.subnet_id
      assign_public_ip = var.pg_public_ip
    }
  }
}
