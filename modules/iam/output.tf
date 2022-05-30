output "cluster_service_account_id" {
  value = yandex_iam_service_account.cluster.id
}
output "cluster_node_service_account_id" {
  value = yandex_iam_service_account.cluster_node.id
}
output "monitoring_sa_api_key" {
  value = yandex_iam_service_account_api_key.monitoring-sa-api-key.secret_key
}
