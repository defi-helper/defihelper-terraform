resource "yandex_mdb_mysql_cluster" "mysql" {
  name        = var.name
  environment = "PRESTABLE"
  network_id  = var.vpc_id
  version     = var.mysql_version

  resources {
    resource_preset_id = var.mysql_tier_id
    disk_type_id       = "network-ssd"
    disk_size          = var.mysql_instance_disk_size
  }

  mysql_config = {
    sql_mode                      = "ONLY_FULL_GROUP_BY,STRICT_TRANS_TABLES,NO_ZERO_IN_DATE,NO_ZERO_DATE,ERROR_FOR_DIVISION_BY_ZERO,NO_ENGINE_SUBSTITUTION"
    max_connections               = 100
    default_authentication_plugin = "MYSQL_NATIVE_PASSWORD"
    innodb_print_all_deadlocks    = true

  }

  access {
    web_sql = true
  }

  database {
    name = "default_db"
  }

  user {
    name     = var.mysql_admin_name
    password = var.mysql_admin_password
    connection_limits {
      max_user_connections = var.mysql_admin_conn_limit
    }
    permission {
      database_name = "default_db"
      roles         = ["ALL"]
    }
    permission {
      database_name = "antitreningi"
      roles         = ["ALL"]
    }
    permission {
      database_name = "oplatakursov"
      roles         = ["ALL"]
    }
  }

  database {
    name = "antitreningi"
  }

  user {
    name     = var.mysql_antitreningi_name
    password = var.mysql_antitreningi_password
    connection_limits {
      max_user_connections = var.mysql_antitreningi_conn_limit
    }
    permission {
      database_name = "antitreningi"
      roles         = ["ALL"]
    }
  }

  database {
    name = "oplatakursov"
  }

  user {
    name     = var.mysql_oplatakursov_name
    password = var.mysql_oplatakursov_password
    connection_limits {
      max_user_connections = var.mysql_oplatakursov_conn_limit
    }
    permission {
      database_name = "oplatakursov"
      roles         = ["ALL"]
    }
  }

  dynamic "host" {
      for_each = var.enable_replication ? [for conf in var.location_subnets:{
          zone = conf.zone
          subnet_id = conf.id
      }] : [{
          zone = var.location_subnets[0].zone
          subnet_id = var.location_subnets[0].id
      }]

      content {
          zone = host.value.zone
          subnet_id = host.value.subnet_id
          assign_public_ip = false
      }
  }
}