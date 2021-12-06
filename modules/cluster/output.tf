output "node_group_ids" {
  value = module.node_groups.node_group_ids
}
output "external_v4_endpoint" {
  value = var.enable_replication ? yandex_kubernetes_cluster.regional_cluster[0].master[0].external_v4_endpoint : yandex_kubernetes_cluster.zonal_cluster[0].master[0].external_v4_endpoint
}
output "ca_certificate" {
  value = var.enable_replication ? yandex_kubernetes_cluster.regional_cluster[0].master[0].cluster_ca_certificate : yandex_kubernetes_cluster.zonal_cluster[0].master[0].cluster_ca_certificate
}
