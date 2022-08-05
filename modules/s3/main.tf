// Create SA
resource "yandex_iam_service_account" "sa_s3" {
  folder_id = var.folder_id
  name      = var.s3_service_account
}

// Grant permissions
resource "yandex_resourcemanager_folder_iam_member" "sa-admin" {
  folder_id = var.folder_id
  role      = "storage.admin"
  member    = "serviceAccount:${yandex_iam_service_account.sa_s3.id}"
}

// Create Static Access Keys
resource "yandex_iam_service_account_static_access_key" "sa-static-key" {
  service_account_id = yandex_iam_service_account.sa_s3.id
  description        = "static access key for object storage"
}

resource "yandex_storage_bucket" "loki_bucket" {
  access_key = yandex_iam_service_account_static_access_key.sa-static-key.access_key
  secret_key = yandex_iam_service_account_static_access_key.sa-static-key.secret_key

  bucket = var.loki_bucket_name

  grant {
    id          = yandex_iam_service_account.sa_s3_loki.id
    type        = "CanonicalUser"
    permissions = ["FULL_CONTROL"]
  }
}

resource "yandex_storage_bucket" "open_bucket" {
  access_key = yandex_iam_service_account_static_access_key.sa-static-key.access_key
  secret_key = yandex_iam_service_account_static_access_key.sa-static-key.secret_key

  bucket = var.open_bucket_name

  grant {
    id          = yandex_iam_service_account.sa_s3.id
    type        = "CanonicalUser"
    permissions = ["FULL_CONTROL"]
  }
}

resource "yandex_storage_bucket" "open_public_bucket" {
  access_key = yandex_iam_service_account_static_access_key.sa-static-key.access_key
  secret_key = yandex_iam_service_account_static_access_key.sa-static-key.secret_key

  bucket = "${var.open_bucket_name}-public"

  grant {
    id          = yandex_iam_service_account.sa_s3.id
    type        = "CanonicalUser"
    permissions = ["FULL_CONTROL"]
  }

  grant {
    type        = "Group"
    permissions = ["READ"]
    uri         = "http://acs.amazonaws.com/groups/global/AllUsers"
  }
}

resource "yandex_iam_service_account" "sa_s3_loki" {
  name        = var.s3_service_account_loki
  description = "S3 service account"
}

resource "yandex_iam_service_account_static_access_key" "sa-loki-static-key" {
  service_account_id = yandex_iam_service_account.sa_s3_loki.id
  description        = "static key for authorization"
}
