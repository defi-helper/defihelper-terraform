resource "yandex_kubernetes_cluster" "regional_cluster" {
  count         = var.enable_replication ? 1 : 0
  name = var.name

  network_id = var.vpc_id

  master {
    regional {
      region = var.region

      dynamic "location" {
        for_each = var.location_subnets

        content {
          zone = location.value.zone
          subnet_id = location.value.id
        }
      }
    }

    version = var.kube_version
    public_ip = var.public

    maintenance_policy {
      auto_upgrade = false
    }
  }

  service_account_id = var.cluster_service_account_id
  node_service_account_id = var.node_service_account_id

  release_channel = var.release_channel

  depends_on = [
    var.dep
  ]
}

resource "yandex_kubernetes_cluster" "zonal_cluster" {
  count         = var.enable_replication ? 0 : 1
  name = var.name

  network_id = var.vpc_id

  master {
    zonal {
      zone = var.location_subnets[0].zone
      subnet_id = var.location_subnets[0].id
    }

    version = var.kube_version
    public_ip = var.public

    maintenance_policy {
      auto_upgrade = false
    }
  }

  service_account_id = var.cluster_service_account_id
  node_service_account_id = var.node_service_account_id

  release_channel = var.release_channel

  depends_on = [
    var.dep
  ]
}

module "node_groups" {
  source = "./modules/node_groups"

  cluster_id  = var.enable_replication ? yandex_kubernetes_cluster.regional_cluster[0].id : yandex_kubernetes_cluster.zonal_cluster[0].id

  kube_version = var.kube_version
  location_subnets = var.location_subnets
  cluster_node_groups = var.cluster_node_groups
  ssh_keys = var.ssh_keys
}
