export class SequenceIterable {

    constructor(object) {
        this.object = object
    }

    [Symbol.iterator]() {
        const object = this.object
        let currentItem = object.firstItem
        return {
            next() {
                if (currentItem === null) {
                    return { done: true }
                }
                
                const value = currentItem
                currentItem = object.nextItem(currentItem)
                
                return { 
                    value: value,
                    done: false 
                }
            }
        }
    }

}

export class ListIterable {
    constructor(object) {
        this.object = object
    }

    [Symbol.iterator]() {
        const object = this.object
        const items = object.items
        let index = 0
        return {
            next() {
                if (index >= items.length) {
                    return { done: true }
                }

                const value = items[index]
                index++

                return {
                    value: value,
                    done: false
                }
            }
        }
    }
}

export class MapIterable {
    constructor(object) {
        this.object = object
    }

    [Symbol.iterator]() {
        const object = this.object
        const keys = object.keys
        let index = 0
        return {
            next() {
                if (index >= keys.length) {
                    return { done: true }
                }

                const key = keys[index]
                const value = object.item(key)
                index++

                return {
                    value: [key, value],
                    done: false
                }
            }
        }
    }
}