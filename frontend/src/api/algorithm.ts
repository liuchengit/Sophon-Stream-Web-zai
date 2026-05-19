import { get, post, put, del } from './request'

export interface Algorithm {
  id: number
  name: string
  type: 'detection' | 'tracking' | 'classification' | 'segmentation' | 'pose' | 'ocr' | 'face'
  modelPath: string
  version: string
  inputSize: string
  classes: string[]
  status: 'active' | 'inactive'
  description: string
  config: Record<string, any>
  createdAt: string
  updatedAt: string
}

export interface AlgorithmFormData {
  name: string
  type: string
  modelPath: string
  version: string
  inputSize: string
  classes: string[]
  description: string
  config: Record<string, any>
}

export function fetchAlgorithms(params?: Record<string, any>) {
  return get<{ items: Algorithm[]; total: number }>('/algorithms', params)
}

export function fetchAlgorithm(id: number) {
  return get<Algorithm>('/algorithms', { id })
}

export function createAlgorithm(data: AlgorithmFormData) {
  return post<Algorithm>('/algorithms', data)
}

export function updateAlgorithm(id: number, data: Partial<AlgorithmFormData>) {
  return put<Algorithm>('/algorithms', { ...data, id })
}

export function deleteAlgorithm(id: number) {
  return del('/algorithms', { id })
}

export function validateModel(id: number) {
  return post<{ valid: boolean; message: string }>('/algorithms/validate', { id })
}

export function getAlgorithmTypes() {
  return get<string[]>('/algorithms/types')
}
