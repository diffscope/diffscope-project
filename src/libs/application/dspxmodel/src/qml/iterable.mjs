export class Iterable {

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