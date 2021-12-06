terraform {
  required_version = ">= 1.0.2"

  required_providers {
    yandex = {
      source  = "yandex-cloud/yandex"
      version = "~> 0.61.0"
    }
  }
}
