data "yandex_resourcemanager_folder" "cluster_folder" {
  folder_id = var.cluster_folder_id
}

resource "yandex_iam_service_account" "cluster" {
  name = var.cluster_service_account_name
}

resource "yandex_resourcemanager_folder_iam_member" "cluster-admin" {
  folder_id = data.yandex_resourcemanager_folder.cluster_folder.id
  role   = "editor"
  member = "serviceAccount:${yandex_iam_service_account.cluster.id}"
}

resource "yandex_iam_service_account" "cluster_node" {
  name = var.cluster_node_service_account_name
}

resource "yandex_resourcemanager_folder_iam_member" "cluster_node-admin" {
  folder_id = data.yandex_resourcemanager_folder.cluster_folder.id
  role   = "container-registry.images.puller"
  member = "serviceAccount:${yandex_iam_service_account.cluster_node.id}"
}

resource "yandex_iam_service_account" "container_registry" {
  name = var.container_registry_sa_name
}

resource "yandex_resourcemanager_folder_iam_member" "container_registry_admin" {
  folder_id = data.yandex_resourcemanager_folder.cluster_folder.id
  role   = "container-registry.admin"
  member = "serviceAccount:${yandex_iam_service_account.container_registry.id}"
}

resource "yandex_iam_service_account" "monitoring" {
  name = var.monitoring_sa_name
}

resource "yandex_resourcemanager_folder_iam_member" "monitoring_admin" {
  folder_id = data.yandex_resourcemanager_folder.cluster_folder.id
  role   = "monitoring.viewer"
  member = "serviceAccount:${yandex_iam_service_account.monitoring.id}"
}

resource "yandex_iam_service_account_api_key" "monitoring-sa-api-key" {
  service_account_id = yandex_iam_service_account.monitoring.id
  description        = "api key for monitoring"
}
