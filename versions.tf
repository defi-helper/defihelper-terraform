terraform {
  required_version = ">= 1.0.2"

  required_providers {
    yandex = {
      source  = "yandex-cloud/yandex"
      version = "~> 0.61.0"
    }
    kubernetes = {
      source = "hashicorp/kubernetes"
      version = "2.16.1"
    }
    kubectl = {
      source  = "gavinbunney/kubectl"
      version = "1.10.0"
    }
    helm = {
      source  = "hashicorp/helm"
      version = "2.2.0"
    }
    http = {
      source = "hashicorp/http"
    }
    local = {
      source = "hashicorp/local"
    }
    random = {
      source = "hashicorp/random"
    }
  }

  backend "remote" {
    hostname     = "app.terraform.io"
    organization = "adcorn"

    workspaces {
      name = "defihelper-development"
    }
  }
}
