output "s3_loki_static_access_key" {
  value = yandex_iam_service_account_static_access_key.sa-loki-static-key.access_key
}

output "s3_loki_static_secret_key" {
  value = yandex_iam_service_account_static_access_key.sa-loki-static-key.secret_key
}
